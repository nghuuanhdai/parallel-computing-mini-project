DCDB_PREFIX=/home/nnhhaadd/DCDB/install
UN=$(uname)

# Set LD_LIBRARY_PATH (DYLD_LIBRARY_PATH on MacOs)
if [ "${UN}" = "Darwin" ]; then
	export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:${DCDB_PREFIX}/lib/
else
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${DCDB_PREFIX}/lib/
fi

# Set PATH
export PATH=$PATH:${DCDB_PREFIX}/bin/

# Enable bash completion for dcbdconfig
_dcdbconfig_options()
{
	local curr_arg num_args hostname_str toplevel_command_at comrep
	curr_arg=${COMP_WORDS[COMP_CWORD]}
	num_args=${#COMP_WORDS[@]}
	
	comrep=""
	hostname_str=""
	toplevel_command_at=1
	
	if [ "${COMP_WORDS[1]:0:2}" = "-h" ]; then
		if [ "${COMP_WORDS[1]}" = "-h" ]; then
			toplevel_command_at=3
			hostname_str="-h ${COMP_WORDS[2]}"
		else
			toplevel_command_at=2
			hostname_str="${COMP_WORDS[1]}"
		fi
	else
		if [ "${num_args}" -le "2" ]; then
			comrep+="-h "
		fi
	fi
	
	if [ "${num_args}" -le "$((${toplevel_command_at}+1))" ]; then
		comrep+="help sensor db job"
	else
		if [ "${COMP_WORDS[${toplevel_command_at}]}" = "help" ]; then
			if [ ! "${COMP_WORDS[$((${COMP_CWORD}-1))]}" = "sensor" ]; then
				comrep="sensor db job"
			fi
		elif [ "${COMP_WORDS[${toplevel_command_at}]}" = "sensor" ]; then
			if [ "${num_args}" -eq "$((${toplevel_command_at}+2))" ]; then
				comrep="publish vcreate list listpublic show scalingfactor unit ttl integrable expression operations clearoperations clearoperationsw tzero frequency unpublish unpublishw"
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "show" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "scalingfactor" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "unit" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "integrable" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "integrable" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+4))" ]; then
				comrep="on off"
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "expression" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "tzero" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "frequency" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "unpublish" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
			if [ "${COMP_WORDS[$((${toplevel_command_at}+1))]}" = "unpublishw" ] && [ "${num_args}" -eq "$((${toplevel_command_at}+3))" ]; then
				comrep=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
			fi
        elif [ "${COMP_WORDS[${toplevel_command_at}]}" = "db" ]; then
            if [ "${num_args}" -eq "$((${toplevel_command_at}+2))" ]; then
                comrep="insert fuzzytrunc"
            fi
        elif [ "${COMP_WORDS[${toplevel_command_at}]}" = "job" ]; then
            if [ "${num_args}" -eq "$((${toplevel_command_at}+2))" ]; then
                comrep="show list running pending finished"
            fi
		fi
	fi

	COMPREPLY=( $(compgen -W '${comrep}' -- $curr_arg ) )
}
complete -F _dcdbconfig_options dcdbconfig

# Enable bash completion on dcdbquery
_dcdbquery_options()
{
	local i curr_arg num_args hostname_str sensorlist_at rawreq localreq comrep
	
	curr_arg=${COMP_WORDS[COMP_CWORD]}
	num_args=${#COMP_WORDS[@]}
	
	comrep=""
	hostname_str=""
	sensorlist_at=1
	rawreq=0
	localreq=0
	floatreq=0
	i=0
	
	# Go through all possible options until first non-option (i.e. sensor) is found
	while [ "$i" -le "$num_args" ] && [ "$i" -le "5" ]; do
		if [ "${COMP_WORDS[$i]}" = "-r" ]; then
			rawreq=1
			sensorlist_at=$((sensorlist_at+1))
        elif [ "${COMP_WORDS[$i]}" = "-l" ]; then
            localreq=1
            sensorlist_at=$((sensorlist_at+1))
        elif [ "${COMP_WORDS[$i]}" = "-f" ]; then
            floatreq=1
            sensorlist_at=$((sensorlist_at+1))
		elif [ "${COMP_WORDS[$i]:0:2}" = "-h" ]; then
			if [ "${COMP_WORDS[$i]}" = "-h" ]; then
				i=$((i+1))
				sensorlist_at=$((sensorlist_at+2))
				hostname_str="-h ${COMP_WORDS[$i]}"
			else
				hostname_str="${COMP_WORDS[$i]}"
				sensorlist_at=$((sensorlist_at+1))
			fi
		fi

		i=$((i+1))
	done
	
	if [ "$((${num_args}-1))" -le "${sensorlist_at}" ]; then
		if [ ! "${rawreq}" -eq "1" ]; then
			comrep+=" -r"
		fi
		if [ ! "${localreq}" -eq "1" ]; then
			comrep+=" -l"
		fi
        if [ ! "${floatreq}" -eq "1" ]; then
            comrep+=" -f"
        fi
		if [ "${hostname_str}" = "" ]; then
			comrep+=" -h"
		fi
	fi
	
	comrep+=" "
	comrep+=$(dcdbconfig ${hostname_str} sensor list 2> /dev/null)
	
#	echo
#	echo "Num args: $num_args"
#	echo "Sensorlist at: $sensorlist_at"
#	echo "Hostname_str: $hostname_str"
#	echo "Hostname_missing: $hostname_missing"
	
	if [ "${hostname_str}" = "-h " ]; then
		comrep=""
	fi
	
	COMPREPLY=( $(compgen -W '${comrep}' -- $curr_arg ) )
}
complete -F _dcdbquery_options dcdbquery
