/* BEGIN LICENSE
 * Copyright (C) 2011 Percona Inc.
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 * END LICENSE */

#ifndef PERCONA_PLAYBACK_QUERY_RESULT_H
#define PERCONA_PLAYBACK_QUERY_RESULT_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include "percona_playback/visibility.h"

#ifdef __cplusplus
extern "C"
{
#endif

class QueryResult {
 private:
  uint64_t _rows_sent;
  uint64_t _rows_examined;
  int _error;
  int _warning_count;
  boost::posix_time::time_duration _duration;

 public:
  QueryResult() :  _rows_sent(0),
    _rows_examined(0),
    _error(0),
    _warning_count(0)
  {
  }

  void setRowsSent(const uint64_t &rows) {
    _rows_sent= rows;
  }

  void setRowsExamined(const uint64_t &rows) {
    _rows_examined= rows;
  }

  void setError(int e) {
    _error= e;
  }

  void setWarningCount(int nr) {
    _warning_count= nr;
  }

  void setDuration(const boost::posix_time::time_duration &duration) {
    _duration= duration;
  }

  uint64_t getRowsSent() const {
    return _rows_sent;
  }

  uint64_t getRowsExamined() const {
    return _rows_examined;
  }

  int getError() const {
    return _error;
  }

  int getWarningCount() const {
    return _warning_count;
  }

  boost::posix_time::time_duration getDuration() const {
    return _duration;
  }

};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_QUERY_RESULT_H */