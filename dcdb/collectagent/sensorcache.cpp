//================================================================================
// Name        : sensorcache.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2016-2019 Leibniz Supercomputing Centre
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

#include "sensorcache.h"

SensorCache::SensorCache(uint64_t maxHistory) {
    this->_maxHistory = maxHistory;
    this->_updating.store(false);
    this->_access.store(0);
}

SensorCache::~SensorCache() {
    sensorCache.clear();
}

sensorCache_t& SensorCache::getSensorMap() {
    return sensorCache;
}

void SensorCache::storeSensor(SensorId sid, uint64_t ts, int64_t val) {
  reading_t s = { val, ts };
  /* Remove the reserved bytes to leverage the standard find function */
  sid.setRsvd(0);
  wait();
  auto sIt = sensorCache.find(sid);
  if(sIt!=sensorCache.end()) {
      sIt->second.store(s);
      release();
  } else {
      release();
      // Spinning on the lock
      while (_updating.exchange(true)) {}
      while(_access.load()>0) {}
      sensorCache[sid] = CacheEntry(_maxHistory);
      sensorCache[sid].store(s);
      _updating.store(false);
  }
}

void SensorCache::storeSensor(const SensorDataStoreReading& s) {
    storeSensor(s.sensorId, s.timeStamp.getRaw(), s.value);
}

int64_t SensorCache::getSensor(SensorId sid, uint64_t avg) {
  /* Remove the reserved bytes to leverage the standard find function */
  sid.setRsvd(0);
  wait();
  sensorCache_t::iterator it = sensorCache.find(sid);

  if (it == sensorCache.end()) {
    release();
    throw std::invalid_argument("Sid not found");
  }

  if (!it->second.checkValid())
  { 
    release();
    throw std::out_of_range("Sid outdated");
  }

  int64_t val = avg ? it->second.getAverage(avg) : it->second.getLatest().value;
  release();
  return val;
}

// Wildcards are not supported with string topics
//int64_t SensorCache::getSensor(std::string topic, uint64_t avg) {
//  topic.erase(std::remove(topic.begin(), topic.end(), '/'), topic.end());
//
//  size_t wp = topic.find("*");
//  if (wp == std::string::npos) {
//    return getSensor(DCDB::SensorId(topic), avg);
//  }
//
//  int wl = 29 - topic.length();
// 
//  /* Create SensorIds with the lowest and highest values matching the wildcard */
//  DCDB::SensorId sidLow(std::string(topic).replace(wp, 1, std::string(wl, '0')));
//  DCDB::SensorId sidHi(std::string(topic).replace(wp, 1, std::string(wl, 'f')));
//  DCDB::SensorId sidMask(std::string(wp, 'f') + std::string(wl, '0') + std::string(topic.length()-wp-1, 'f'));
//  sidLow.setRsvd(0);
//  sidHi.setRsvd(0);
//
//  /* See whether there's a SensorId in the cache >= sidLow */
//  sensorCache_t::iterator it = sensorCache.lower_bound(sidLow);
//  sensorCache_t::iterator mostRecentSidIt = sensorCache.end();
//
//  bool foundOne = false;
//  /* Iterate over the cache until the current entry is > sidHi */
//  while ((it != sensorCache.end()) && (it->first <= sidHi)) {
//    if ((it->first & sidMask) == sidLow) {
//      foundOne = true;
//      /* We only return one value, even if multiple SensorIds would match.
//       * At least make sure it's the most recent value
//       */
//      if (it->second.checkValid() && ((mostRecentSidIt == sensorCache.end()) || mostRecentSidIt->second.getLatest().timestamp < it->second.getLatest().timestamp)) {
//        mostRecentSidIt = it;
//      }
//    }
//    it++;
//  }
//
//  if (mostRecentSidIt == sensorCache.end()) {
//    /* Check whether we actually found at least an outdated entry */
//    if (foundOne) {
//      throw std::out_of_range("Sid outdated");
//    } else {
//      throw std::invalid_argument("Sid not found");
//    }
//  }
//
//  if (avg) {
//    return mostRecentSidIt->second.getAverage(avg);
//  } else {
//    return mostRecentSidIt->second.getLatest().value;
//  }
//}

void SensorCache::dump() {
  std::cout << "SensorCache Dump:" << std::endl;
  for (sensorCache_t::iterator sit = sensorCache.begin(); sit != sensorCache.end(); sit++) {
    std::cout << "  id=" << sit->first.getId() << " data=[";
    for (std::vector<reading_t>::const_iterator eit = sit->second.getRaw()->begin(); eit != sit->second.getRaw()->end(); eit++) {
      if (eit != sit->second.getRaw()->begin()) {
        std::cout << ",";
      }
      std::cout << "(" <<  eit->value << "," << eit->timestamp/NS_PER_S << "." << std::setfill ('0') << std::setw (9) << eit->timestamp%NS_PER_S << ")";
    }
    std::cout << "]" << std::endl;
  }
}

uint64_t SensorCache::clean(uint64_t t) {
    uint64_t thresh = getTimestamp() - t;
    uint64_t ctr = 0;
    // Spinning on the lock
    while (_updating.exchange(true)) {}
    while(_access.load()>0) {}
    for (auto it = sensorCache.cbegin(); it != sensorCache.cend();) {
        uint64_t latestTs = it->second.getLatest().timestamp;
        if (latestTs!=0 && latestTs < thresh) {
            it = sensorCache.erase(it);
            ctr++;
        } else
            ++it;
    }
    _updating.store(false);
    return ctr;
}
