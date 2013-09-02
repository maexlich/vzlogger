/**
 * Read data from modbus Servers via TCP/IP
 *
 * @package vzlogger
 * @copyright Copyright (c) 2013, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Max Horn <maexlich@gmail.com>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _MMODBUS_H_
#define _MMODBUS_H_

#include <protocols/Protocol.hpp>
#include "Options.hpp"
#include <modbus.h>

#define READ_COIL_STATUS 1
#define READ_INPUT_STATUS 2
#define READ_HOLDING_REGISTERS 3
#define READ_INPUT_REGISTERS 4

class MeterModbus : public vz::protocol::Protocol {

public:
	MeterModbus(std::list<Option> options);
	virtual ~MeterModbus();
	
	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);
	const char *ip() { return _ip.c_str(); }
  
  private:
	modbus_t *_mb;
	std::string _ip;
	int _port;
	int _address;
	int _length;
	bool _input_read;
	bool _reset_connection;
	struct addressparam *_addressparams;
	void getHighestDigit(unsigned int number, unsigned char *digit, unsigned char *power);
};

#endif /* _FILE_H_ */
