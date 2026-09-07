/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef SRSRAN_RWLOCK_GUARD_H
#define SRSRAN_RWLOCK_GUARD_H
#include <iostream>
#include <pthread.h>

namespace srsran {

class rwlock_write_guard
{
public:
  rwlock_write_guard(pthread_rwlock_t& rwlock_) : rwlock(&rwlock_) { 
    // std::cout<<"thread try w:"<<pthread_self()<<std::endl;
    pthread_rwlock_wrlock(rwlock); 
    // std::cout<<"thread acquired w:"<<pthread_self()<<std::endl;
    }
  rwlock_write_guard(const rwlock_write_guard&) = delete;
  rwlock_write_guard(rwlock_write_guard&&)      = delete;
  rwlock_write_guard& operator=(const rwlock_write_guard&) = delete;
  rwlock_write_guard& operator=(rwlock_write_guard&&) = delete;
  ~rwlock_write_guard() { 
    // std::cout<<"thread try unlock w:"<<pthread_self()<<std::endl;
    pthread_rwlock_unlock(rwlock); 
    // std::cout<<"thread unlock success w:"<<pthread_self()<<std::endl;
    }

private:
  pthread_rwlock_t* rwlock;
};

// Shared lock guard that automatically unlocks rwlock on exit
class rwlock_read_guard
{
public:
  rwlock_read_guard(pthread_rwlock_t& rwlock_) : rwlock(&rwlock_) { 
    // std::cout<<"thread try r:"<<pthread_self()<<std::endl;
    pthread_rwlock_rdlock(rwlock); 
    // std::cout<<"thread acquired r:"<<pthread_self()<<std::endl;
    }
  rwlock_read_guard(const rwlock_read_guard&) = delete;
  rwlock_read_guard(rwlock_read_guard&&)      = delete;
  rwlock_read_guard& operator=(const rwlock_read_guard&) = delete;
  rwlock_read_guard& operator=(rwlock_read_guard&&) = delete;
  ~rwlock_read_guard() { 
    // std::cout<<"thread try unlock r:"<<pthread_self()<<std::endl;
    pthread_rwlock_unlock(rwlock);
    // std::cout<<"thread unlock success r:"<<pthread_self()<<std::endl;
    }

private:
  pthread_rwlock_t* rwlock;
};

} // namespace srsran

#endif // SRSRAN_RWLOCK_GUARD_H
