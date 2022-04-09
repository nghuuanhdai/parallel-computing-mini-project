// This program is to be used with Caliper.
// Caliper belongs to Lawrence Livermore National Security, LLC.
// LLNL-CODE-678900
// All rights reserved.
//
// For LICENSE and other details of Caliper see:
// https://github.com/scalability-llnl/Caliper

//================================================================================
// Name        : DcdbPusher.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Caliper service to forward snapshot data to dcdb pusher.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

/**
 * @file DcdbPusher.cpp
 *
 * @brief Caliper service to forward snapshot data to dcdb pusher.
 *
 * @details Service for the runtime processing level. Relies on the sampler,
 *          symbollookup and timestamp service. To avoid a detour to the flush
 *          level the symbollookup service is invoked manually when processing a
 *          snapshot. This way all required information is available without
 *          having to explicitly trigger a flush and the relevant data can be
 *          instantly forwarded. Unneeded atttributes are stripped off and
 *          discarded.
 *
 */

#include "caliper/CaliperService.h"

#include "caliper/Caliper.h"
#include "caliper/SnapshotRecord.h"

#include "caliper/common/Log.h"
#include "caliper/common/RuntimeConfig.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cxxabi.h>
#include <errno.h>
#include <fcntl.h>
#include <features.h>
#include <sched.h>
#include <semaphore.h>
#include <set>
#include <shared_mutex>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <gelf.h>
#include <libelf.h>

using namespace cali;

namespace {

//TODO move definitions into common header file for cali service and pusher plugin
#define MSGQ_SIZE 16 * 1024 * 1024
#define STR_PREFIX "/cali_dcdb_"
#define SHM_SIZE (17 * 1024 * 1024)
#define SOCK_NAME "DCDBPusherCaliSocket"

/*
 * Queue entry layout:
 * {
 *   uint64_t timestamp
 *   char[]   data (NUL terminated string with at most max_dat_size bytes)
 * }
 */

// each thread has a local buffer for relevant snapshot data to reduce
// writes to shm queue as this requires locking
static constexpr size_t    shm_buf_size = 32 * 1024;
static constexpr size_t    max_dat_size = shm_buf_size - sizeof(uint64_t);
static thread_local size_t shm_buf_idx;
static thread_local char   shm_buf[shm_buf_size];

class DcdbPusher {

	struct func_symbol {
		uintptr_t   start_addr;
		uintptr_t   end_addr;
		std::string name;

		/* make it sortable for faster lookups */
		bool operator<(const func_symbol &rhs) const {
			return end_addr < rhs.end_addr;
		}
	};

	/* Defines a contiguous executable memory block */
	struct addr_range {
		uintptr_t             start_addr;
		uintptr_t             end_addr;
		std::string           pathname;
		std::set<func_symbol> symbols; // Filepath + name of the binary where
					       // this memory range comes from or
					       // "[Anonymous]" if unknown

		/* make it sortable for faster lookups */
		bool operator<(const addr_range &rhs) const {
			return end_addr < rhs.end_addr;
		}
	};

      private:
	static const ConfigSet::Entry s_configdata[];

	/* General service attributes */
	unsigned snapshots_processed = 0;
	unsigned snapshots_failed = 0;
	unsigned snapshots_sampler = 0;
	unsigned snapshots_event = 0;

	Attribute sampler_pc{Attribute::invalid};
	Attribute event_begin{Attribute::invalid};
	Attribute event_set{Attribute::invalid};
	Attribute event_end{Attribute::invalid};
	Attribute timestamp{Attribute::invalid};
	Attribute thread_id{Attribute::invalid};

	/* We store information about function symbols and their addresses here */
	std::set<addr_range>      addr_data;
	mutable std::shared_mutex symbol_lock;

	/*
     * Layout of shared-memory file used to communicate with dcdbpusher plugin:
     *
     * //Communication queue, aka ring buffer:
     * size_t r_index //points to the last read byte
     * size_t w_index //points to the last written byte
     * sem_t  r_sem; // atomic access to r_index
     * sem_t  w_sem; // atomic access to w_index
     * char[MSGQ_SIZE] //contains variable length entries
     * //TODO do not exceed SHM_SIZE
     */

	//TODO investigate consequences of fork() on Caliper - DCDBPusher construct
	void *shm;      // pointer to shared memory object
	int   shm_file; // fd of the underlying shared memory file
	int   sock;     // unix socket fd for initial shm setup communication

	size_t            sus_cycle = 15;
	std::atomic<bool> run_sus;
	std::atomic<bool> sus_trigger;
	std::thread       sus; //symbol update service

	const std::string pid_str; // PID at process start
	bool              initialized;

	/*
     * Retrieve function symbols from an ELF file and store them in the given
     * set.
     *
     * @param filename   ELF file (binary or shared library) to retrieve symbol info from
     * @param start_addr Only store symbols whose address is in between start_addr and end_addr
     * @param end_addr   See start_addr
     * @param offset     Offset of start_addr from the beginning of the file
     * @param dest       Set in which to store the function symbol data.
     * @param chn        Caliper Channel for logging output.
     * @return
     *
     * --NOTE--
     * As one could expect this custom symbol lookup routine is not very mature.
     * The found symbols and their addresses seem to be calculated correctly.
     * However, the symbol addresses calculated by gdb and this routine are
     * slightly off *sometimes* (seldom). In this case gdb can still resolve our
     * calculated address to the correct symbol name. Currently I lack the
     * knowledge to explain this discrepancy, therefore I leave a TODO here as
     * short-term legacy.
     * Also symbol information can be incomplete for stripped binaries. But as
     * this program is only intended for introspection into libraries controlled
     * by us, we do not care about stripped binaries too much.
     * We skip support for separate debug symbols. Unstripped binaries should
     * provide us with enough symbol information.
     */
	void write_function_symbols(const std::string      filename,
				    const uintptr_t        start_addr,
				    const uintptr_t        end_addr,
				    const uintptr_t        offset,
				    std::set<func_symbol> &dest,
				    Channel *              chn) {
		Elf *     elf;
		Elf_Scn * scn = NULL;
		GElf_Ehdr ehdr;
		GElf_Shdr shdr;
		Elf_Data *data;
		int       fd;
		uintptr_t sym_offset;

		elf_version(EV_CURRENT);

		fd = open(filename.c_str(), O_RDONLY);
		if (fd == -1) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Could not open ELF file: "
					<< strerror(errno) << std::endl;
			return;
		}

		//search ELF header for symbol table
		elf = elf_begin(fd, ELF_C_READ, NULL);

		gelf_getehdr(elf, &ehdr);
		//check if ELF file type is supported
		if (ehdr.e_type != ET_DYN && ehdr.e_type != ET_EXEC) {
			// we should only encounter executables and shared libraries during runtime
			// we could not process other types anyway
			Log(1).stream() << chn->name() << ": DcdbPusher: Unknown ELF type" << std::endl;
			return;
		}
		sym_offset = (ehdr.e_type == ET_DYN ? (start_addr - offset) : 0);

		//search for symtab (= symbol table) section
		while ((scn = elf_nextscn(elf, scn)) != NULL) {
			gelf_getshdr(scn, &shdr);
			if (shdr.sh_type == SHT_SYMTAB) {
				// found symbol table
				data = elf_getdata(scn, NULL);
				break;
			}
		}

		//check if symtab section found and if it is usable
		if (scn == NULL || shdr.sh_entsize == 0) {
			Log(2).stream() << chn->name() << ": DcdbPusher: \"" << filename
					<< "\": No symbol table present. Falling back to dynamic symtab." << std::endl;
			scn = NULL;
			//Fall back to dynamic symbol table. This one should always be present
			while ((scn = elf_nextscn(elf, scn)) != NULL) {
				gelf_getshdr(scn, &shdr);
				if (shdr.sh_type == SHT_DYNSYM) {
					// found dynamic symbol table
					data = elf_getdata(scn, NULL);
					break;
				}
			}

			if (scn == NULL || shdr.sh_entsize == 0) {
				Log(1).stream() << chn->name() << ": DcdbPusher: Absolutely no symbols found in \""
						<< filename << "\"" << std::endl;
				return;
			}
		}

		//retrieve symbol data
		int count = shdr.sh_size / shdr.sh_entsize;
		//debug
		//        printf("Section has %d symbols\n", count);
		for (int ii = 0; ii < count; ++ii) {
			GElf_Sym sym;

			if (gelf_getsym(data, ii, &sym) == NULL) {
				Log(1).stream() << chn->name() << ": DcdbPusher: Got no symbol" << std::endl;
				continue;
			}

			if (GELF_ST_TYPE(sym.st_info) != STT_FUNC || //only interested in symbols related to executable code
			    sym.st_shndx == SHN_UNDEF ||             //external symbol
			    sym.st_shndx == SHN_ABS) {               //absolute symbol, unlikely for STT_FUNC
				continue;
			}

			//resolve symbol name
			char *symstr;
			char *dsymstr;

			func_symbol symdat;

			symstr = elf_strptr(elf, shdr.sh_link, sym.st_name);

			if (symstr != NULL) {
				// Demangle if necessary. Require GNU v3 ABI by the "_Z" prefix.
				int status = -1;
				if (symstr[0] == '_' && symstr[1] == 'Z') {
					dsymstr = abi::__cxa_demangle(symstr, NULL, NULL, &status);
				}

				if (status == 0) {
					symdat.name = std::string(dsymstr);
					free((void *)dsymstr);
				} else {
					symdat.name = std::string(symstr);
				}
			} else {
				symdat.name = "";
			}

			//resolve symbol value aka its address in this' process virtual memory
			symdat.start_addr = sym_offset + sym.st_value;
			symdat.end_addr = symdat.start_addr + sym.st_size - 1;

			if (symdat.start_addr >= start_addr &&
			    symdat.start_addr <= end_addr) {

				//debug
				//                    printf("Symbol %s in mem range (%llx-%llx, size %llx)\n", symdat.name,
				//                                                                              symdat.start_addr,
				//                                                                              symdat.end_addr,
				//                                                                              sym.st_size);
				if (!dest.insert(symdat).second) {
					//TODO check again here
					//Log(1).stream() << chn->name() << ": DcdbPusher: Could not insert symbol!" << std::endl;
				}
			} else {
				//                printf("Symbol %s out of mem range (%llx-%llx, size %llx)\n", symdat.name,
				//                                                                              symdat.start_addr,
				//                                                                              symdat.end_addr,
				//                                                                              sym.st_size);
			}
		}

		//debug
		//        printf("%s:\n", filename.c_str());
		//        if (shdr.sh_type == SHT_DYNSYM) {
		//            printf("Retrieved %d symbols of dynsym\n", dest.size());
		//        } else {
		//            printf("Retrieved %d symbols of symtab\n", dest.size());
		//        }

		elf_end(elf);
		close(fd);
		return;
	}

	/**
     * Set up address data. Parse all address ranges and their pathnames
     * which are marked as executable from /proc//maps.
     * Address ranges associated to a binary ELF file will be enriched with
     * symbol data.
     * TODO demangle different languages? (C -> no mangling,
     *                                     C++ -> demangling implemented,
     *                                     Fortran -> mangling compiler dependent; no demangling implemented,
     *                                     other languages?)
     */
	bool read_addr_data(Channel *chn) {
		FILE *     file;
		addr_range addr;
		char       exec;
		uintptr_t  offset;
		char       buf[4096];

		addr_data.clear();

		//read mapped address ranges from /proc/self/maps
		if (!(file = fopen("/proc/self/maps", "r"))) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Could not open memory map: "
					<< strerror(errno) << std::endl;
			return false;
		}

		//read one line = one address range
		while (fscanf(file, "%llx-%llx %*2c%1c%*s%llx%*s%*s%4096[^\n]",
			      &(addr.start_addr),
			      &(addr.end_addr),
			      &exec,
			      &(offset),
			      buf) == 5) {

			//debug
			//printf("%llx-%llx %c %llx %s\n", addr.start_addr, addr.end_addr, exec, offset, buf);

			//Only executable memory ranges are interesting. If the program counter
			//ever points in a non-executable section --> HCF
			if (exec == 'x') {
				addr.pathname = std::string(buf);

				//get rid of leading whitespaces
				addr.pathname.erase(0, addr.pathname.find_first_not_of(" \t\n\r\f\v"));

				//mem ranges are not required to be associated with a name
				if (addr.pathname.empty()) {
					addr.pathname = "[Anonymous]";
				} else if (addr.pathname[0] == '/') {
					//address ranges associated with a binary file get enriched
					//with symbol data

					//debug
					//printf("Parsing symbols for %s (%llx-%llx; %llx)\n", addr.pathname.c_str(), addr.start_addr, addr.end_addr, offset);
					write_function_symbols(addr.pathname,
							       addr.start_addr,
							       addr.end_addr,
							       offset,
							       addr.symbols,
							       chn);
				}

				//forward slashes are reserved for MQTT topics, use double colon instead
				std::replace(addr.pathname.begin(), addr.pathname.end(), '/', ':');

				//save this range
				if (!addr_data.insert(addr).second) {
					Log(1).stream() << chn->name() << ": DcdbPusher: Could not insert address range!" << std::endl;
					fclose(file);
					return false;
				}
			}
		}
		fclose(file);
		//        Log(1).stream() << chn->name() << ": DcdbPusher: Scan error: "
		//                        << strerror(errno) << std::endl;

		return true;
	}

	/**
     * Stop symbol update service background thread.
     */
	void stopSUS() {
		if (run_sus.load(std::memory_order_acquire)) {
			run_sus.store(false, std::memory_order_release);
			if (sus.joinable()) {
				sus.join();
			}
		}
	}

	/**
     * Symbol update service background thread. Whenever a PC value could not
     * be resolved it notifies us and this thread comes into
     * play. Updates the symbol data in the shared memory file.
     */
	void startSUS(Channel *chn) {
		stopSUS();
		run_sus.store(true, std::memory_order_release);
		sus = std::thread([=]() {
			//To increase responsiveness on termination we sleep only 1 second at most.
			//However, to reduce update overhead we only check the update flag every 15 seconds
			unsigned short cnt = 0;
			while (run_sus.load(std::memory_order_acquire)) {
				if (cnt == sus_cycle) {

					if (sus_trigger.load(std::memory_order_acquire)) {
						Log(2).stream() << chn->name() << ": DcdbPusher: Updating symbol index" << std::endl;
						symbol_lock.lock();
						if (!read_addr_data(chn)) {
							Log(1).stream() << chn->name() << ": DcdbPusher: Failed to setup shm"
									<< std::endl;
						} else {
							sus_trigger.store(false, std::memory_order_release);
						}
						symbol_lock.unlock();
					}

					cnt = 0;
				}

				++cnt;
				//TODO best interval value? make it dynamic?
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		});
	}

	void print_debug_shm() {
		size_t sym_cnt;
		for (const auto &a : addr_data) {
			printf("Mem range %s: %llx-%llx contains %d symbols:\n", a.pathname,
			       a.start_addr,
			       a.end_addr,
			       a.symbols.size());

			for (const auto &f : a.symbols) {
				printf("> (%llx-%llx) %s\n", f.start_addr,
				       f.end_addr,
				       f.name);
			}
			printf("\n");
			sym_cnt += a.symbols.size();
		}

		printf("%d ranges with overall %d symbols\n", addr_data.size(), sym_cnt);
	}

	/**
     * Writes the thread's snap_data buffer into shm
     */
	bool flush_buf() {
		if (shm_buf_idx == 0) {
			return true;
		}

		size_t r_index;

		sem_t *r_sem;
		sem_t *w_sem;

		char *msg_queue;

		r_sem = reinterpret_cast<sem_t *>(static_cast<char *>(shm) + 2 * sizeof(size_t));
		w_sem = r_sem + 1;
		msg_queue = reinterpret_cast<char *>(w_sem + 1);

		//TODO atomic load/stores instead of semaphore locking?
		if (sem_wait(r_sem)) {
			return false;
		}
		r_index = *(reinterpret_cast<size_t *>(static_cast<char *>(shm)));
		sem_post(r_sem);

		if (sem_trywait(w_sem)) {
			return false;
		}
		size_t &w_index = *(reinterpret_cast<size_t *>(static_cast<char *>(shm) + sizeof(size_t)));

		bool         ret = false;
		const size_t bytes_avail = w_index < r_index ? (r_index - w_index - 1) : (MSGQ_SIZE - w_index + r_index - 1);

		if (bytes_avail >= shm_buf_idx) {
			if ((w_index + shm_buf_idx) >= MSGQ_SIZE) {
				//wrap around end of queue
				size_t sep = MSGQ_SIZE - w_index - 1;
				memcpy(&msg_queue[w_index + 1], shm_buf, sep);
				memcpy(msg_queue, &shm_buf[sep], (shm_buf_idx - sep));
				w_index += shm_buf_idx;
				w_index %= MSGQ_SIZE;
			} else {
				memcpy(&msg_queue[w_index + 1], shm_buf, shm_buf_idx);
				w_index += shm_buf_idx;
			}
			shm_buf_idx = 0;
			ret = true;
		}

		sem_post(w_sem);
		return ret;
	}

	void post_init_cb(Caliper *c, Channel *chn) {
		bool sampler_detected = false;
		bool event_detected = false;

		// Check if required services sampler, event, timestamp and
		// pthread are active by searching for identifying attributes.
		sampler_pc = c->get_attribute("cali.sampler.pc");
		event_begin = c->get_attribute("cali.event.begin");
		event_set = c->get_attribute("cali.event.set");
		event_end = c->get_attribute("cali.event.end");
		timestamp = c->get_attribute("time.timestamp");
		thread_id = c->get_attribute("pthread.id");

		if (timestamp == Attribute::invalid) {
			Log(1).stream() << chn->name() << ": DcdbPusher: required service >timestamp< not running." << std::endl;
			return;
		}

		if (sampler_pc != Attribute::invalid) {
			if (thread_id != Attribute::invalid) {
				sampler_detected = true;
			} else {
				Log(1).stream() << chn->name() << ": DcdbPusher: service >sampler< requires >pthread<" << std::endl;
			}
		}

		if (event_begin != Attribute::invalid &&
		    event_set != Attribute::invalid &&
		    event_end != Attribute::invalid) {
			event_detected = true;
		}

		if (!(sampler_detected || event_detected)) {
			Log(1).stream() << chn->name() << ": DcdbPusher: at least one of the following service combinations is required: "
							  ">sampler,pthread,timestamp< or >event,timestamp<"
					<< std::endl;
			return;
		}

		shm_file = shm_open((STR_PREFIX + pid_str).c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (shm_file == -1) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to open shm_file: "
					<< strerror(errno) << std::endl;
			return;
		}
		if (ftruncate(shm_file, SHM_SIZE)) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to truncate shm_file: "
					<< strerror(errno) << std::endl;
			return;
		}
		shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_file, 0);
		if (shm == (void *)-1) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to mmap shm_file: "
					<< strerror(errno) << std::endl;
			shm = NULL;
			return;
		}

		if (sampler_detected && !read_addr_data(chn)) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to read symbol data"
					<< std::endl;
			return;
		}

		//init r/w_index
		*(reinterpret_cast<size_t *>(static_cast<char *>(shm))) = 0;
		*(reinterpret_cast<size_t *>(static_cast<char *>(shm) + sizeof(size_t))) = 0;

		//init semaphores
		sem_t *r_sem;
		sem_t *w_sem;

		r_sem = reinterpret_cast<sem_t *>(static_cast<char *>(shm) + 2 * sizeof(size_t));
		w_sem = r_sem + 1;

		if (sem_init(r_sem, 1, 1) ||
		    sem_init(w_sem, 1, 1)) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to init semaphore: "
					<< strerror(errno) << std::endl;
			return;
		}

		//init symbol update service if sampler service is running
		if (sampler_detected) {
			startSUS(chn);
		}

		//print_debug_shm();

		//tell pusher plugin our PID so it can access our shared memory
		//UNIX socket used for communication
		sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);

		if (sock == -1) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to open socket: "
					<< strerror(errno) << std::endl;
			return;
		}

		sockaddr_un addr;
		memset(&addr, 0, sizeof(struct sockaddr_un));

		addr.sun_family = AF_UNIX;
		snprintf(&addr.sun_path[1], 91, SOCK_NAME);

		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to connect socket: "
					<< strerror(errno) << std::endl;
			close(sock);
			sock = -1;
			return;
		}

		ssize_t res = send(sock, pid_str.c_str(), pid_str.length() + 1, 0);

		shutdown(sock, SHUT_WR);
		close(sock);
		sock = -1;

		if (res == -1) {
			Log(1).stream() << chn->name() << ": DcdbPusher: Failed to send PID: "
					<< strerror(errno) << std::endl;
			return;
		}

		initialized = true;
	}

	void process_snapshot_cb(Caliper *c, Channel *chn, const SnapshotRecord *, const SnapshotRecord *sbuf) {
		++snapshots_processed;
		// NOTE sampler invoked snapshots are always signals, so better do nothing signal "un-safe" in the case of sampler invoked snapshots
		//if (c->is_signal()) {
		//    ++snapshots_failed;
		//    return;
		//}

		if (!initialized) {
			++snapshots_failed;
			return;
		}

		SnapshotRecord::Sizes sizes = sbuf->size();

		if ((sizes.n_nodes + sizes.n_immediate) == 0) {
			++snapshots_failed;
			return;
		}

		size_t data_size = 0;
		char   data[max_dat_size];

		int cpu;
#if __GLIBC_PREREQ(2, 29)
		unsigned cpuUInt;
		if (getcpu(&cpuUInt, NULL)) {
			Log(1).stream() << chn->name() << ": DcdbPusher: getcpu() failed" << std::endl;
		} else {
			cpu = cpuUInt;
		}
#else
		int cpuInt = sched_getcpu();
		if (cpuInt != -1) {
			cpu = cpuInt;
		} else {
			Log(1).stream() << chn->name() << ": DcdbPusher: sched_getcpu() failed" << std::endl;
		}
#endif

		Entry timestamp_entry = sbuf->get(timestamp);
		Entry sampler_pc_entry = sbuf->get(sampler_pc);
		Entry begin_evt_entry = sbuf->get(event_begin);
		Entry set_evt_entry = sbuf->get(event_set);
		Entry end_evt_entry = sbuf->get(event_end);

		if (!c->is_signal() &&
		    (!begin_evt_entry.is_empty() ||
		     !set_evt_entry.is_empty() ||
		     !end_evt_entry.is_empty())) {
			//must be event, therefore we can also use signal unsafe stuff like std::strings here
			cali_id_t   trigger_id;
			std::string info;

			if (!begin_evt_entry.is_empty()) {
				info = "evt_begin/";
				trigger_id = set_evt_entry.value().to_id();
			} else if (!set_evt_entry.is_empty()) {
				info = "evt_set/";
				trigger_id = begin_evt_entry.value().to_id();
			} else if (!end_evt_entry.is_empty()) {
				info = "evt_end/";
				trigger_id = end_evt_entry.value().to_id();
			} else {
				//should not happen...
				++snapshots_failed;
				return;
			}

			Attribute trigger_attribute = c->get_attribute(trigger_id);
			Entry     trigger_entry = sbuf->get(trigger_attribute);

			info += trigger_attribute.name() + "/" + trigger_entry.value().to_string();

			++snapshots_event;
			data_size = snprintf(data, max_dat_size, "Ecpu%d/%s", cpu, info.c_str());
		} else if (!sampler_pc_entry.is_empty()) {
			//is a sampler-triggered snapshot
			++snapshots_sampler;

			uintptr_t pc = sampler_pc_entry.value().to_uint();

			addr_range a_tmp;
			a_tmp.end_addr = pc;

			if (!symbol_lock.try_lock_shared()) {
				++snapshots_failed;
				return;
			}

			auto a_it = addr_data.lower_bound(a_tmp);
			if (a_it != addr_data.end() && pc >= a_it->start_addr && pc <= a_it->end_addr) {
				//we found associated address range
				func_symbol f_tmp;
				f_tmp.end_addr = pc;

				auto f_it = a_it->symbols.lower_bound(f_tmp);
				if (f_it != a_it->symbols.end() && pc >= f_it->start_addr && pc <= f_it->end_addr) {
					data_size = snprintf(data, max_dat_size, "Scpu%d/%s/%s", cpu, a_it->pathname.c_str(), f_it->name.c_str());
				} else {
					//It's OK if we found no symbol. There are possibly none associated to this range
					data_size = snprintf(data, max_dat_size, "Scpu%d/%s", cpu, a_it->pathname.c_str());
				}
			} else {
				//PC was not within any range --> tell the update service to do his job
				//We let the current snapshot pass unprocessed
				symbol_lock.unlock_shared();
				Log(2).stream() << chn->name() << ": DcdbPusher: symbol index miss. Requesting rebuild" << std::endl;
				sus_trigger.store(true, std::memory_order_release);
				++snapshots_failed;
				return;
			}
			symbol_lock.unlock_shared();
		} else {
			Log(2).stream() << chn->name() << ": DcdbPusher: Snapshot does not match sampler or event" << std::endl;
		}

		//snprintf returns number of bytes written but does not count terminating NUL byte
		++data_size;
		if (data_size > max_dat_size) {
			//snprintf truncated due to size limit
			Log(2).stream() << chn->name() << ": DcdbPusher: data truncated due to size restrictions" << std::endl;
			data_size = max_dat_size;
		}

		uint64_t ts = timestamp_entry.value().to_uint();

		size_t space_avail = shm_buf_size - shm_buf_idx;
		size_t entry_size = sizeof(ts) + data_size;
		if (entry_size > space_avail) {
			if (!flush_buf()) {
				//try again on next snapshot. Will lose this snapshot though
				++snapshots_failed;
				return;
			}
		}

		memcpy(&shm_buf[shm_buf_idx], &ts, sizeof(ts));
		shm_buf_idx += sizeof(ts);
		//must write a NUL terminated string
		memcpy(&shm_buf[shm_buf_idx], (void *)data, data_size);
		shm_buf_idx += data_size;
	}

	void finish_cb(Caliper *c, Channel *chn) {
		initialized = false;
		stopSUS();

		if (shm != NULL) {
			munmap(shm, SHM_SIZE);
			shm = NULL;
		}

		if (shm_file != -1) {
			shm_unlink((STR_PREFIX + pid_str).c_str());
			close(shm_file);
			shm_file = -1;
		}

		Log(1).stream() << chn->name() << ": DcdbPusher: "
				<< snapshots_processed << " snapshots processed of which "
				<< snapshots_failed << " failed ("
				<< snapshots_sampler << " samples, "
				<< snapshots_event << " events)." << std::endl;

		snapshots_processed = 0;
		snapshots_failed = 0;
		snapshots_sampler = 0;
		snapshots_event = 0;
	}

	void create_thread_cb(Caliper *c, Channel *chn) {
		shm_buf_idx = 0;
	}

	void release_thread_cb(Caliper *c, Channel *chn) {
		if (shm != NULL) {
			flush_buf();
		}
	}

	DcdbPusher(Caliper *c, Channel *chn)
	    : shm(NULL),
	      shm_file(-1),
	      sock(-1),
	      run_sus(false),
	      sus_trigger(false),
	      pid_str(std::to_string(getpid())),
	      initialized(false) {
		shm_buf_idx = 0;

		ConfigSet cfg = chn->config().init("dcdbpusher", s_configdata);

		sus_cycle = cfg.get("sus_cycle").to_uint();
	}

      public:
	~DcdbPusher() {}

	static void dcdbpusher_register(Caliper *c, Channel *chn) {
		DcdbPusher *instance = new DcdbPusher(c, chn);

		chn->events().create_thread_evt.connect(
		    [instance](Caliper *c, Channel *chn) {
			    instance->create_thread_cb(c, chn);
		    });
		chn->events().release_thread_evt.connect(
		    [instance](Caliper *c, Channel *chn) {
			    instance->release_thread_cb(c, chn);
		    });
		chn->events().post_init_evt.connect(
		    [instance](Caliper *c, Channel *chn) {
			    instance->post_init_cb(c, chn);
		    });
		chn->events().process_snapshot.connect(
		    [instance](Caliper *c, Channel *chn, const SnapshotRecord *trigger, const SnapshotRecord *snapshot) {
			    instance->process_snapshot_cb(c, chn, trigger, snapshot);
		    });
		chn->events().finish_evt.connect(
		    [instance](Caliper *c, Channel *chn) {
			    instance->finish_cb(c, chn);
			    delete instance;
		    });

		Log(1).stream() << chn->name() << ": Registered dcdbpusher service" << std::endl;
	}
}; // class DcdbPusher

const ConfigSet::Entry DcdbPusher::s_configdata[] = {
    {"sus_cycle", CALI_TYPE_UINT, "15",
     "Symbol update service cycle in seconds (time between checks if update required",
     "Symbol update service cycle in seconds (time between checks if update required"},

    ConfigSet::Terminator};

} // namespace

namespace cali {
CaliperService dcdbpusher_service{"dcdbpusher", ::DcdbPusher::dcdbpusher_register};
}
