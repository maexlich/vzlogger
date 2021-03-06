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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>

#include <protocols/MeterModbus.hpp>
#include <protocols/expression_parser.hpp>
#include <VZException.hpp>
#include <inttypes.h>




MeterModbus::MeterModbus(std::list<Option> options)
		: Protocol("modbus")
{
	OptionList optlist;
	try {
		_ip = optlist.lookup_string(options, "ip");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing IP or invalid type", name().c_str());
		throw;
	}
	try {
		 _port = optlist.lookup_int(options, "port");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing Port or invalid type", name().c_str());
		_port = MODBUS_TCP_DEFAULT_PORT;
	}
	
	//Copy over the addressparams for clean memory management
	struct addressparam *addresses = (struct addressparam *)optlist.lookup_addressparams(options, "addresses");
	struct addressparam *addressptr = addresses;
	_addressparams = addresses;
	
	//addressptr = _addressparams;
	
	while(addressptr->function_code != 0xFF){
		unsigned int length = strlen(addressptr->recalc_str);
		char *str_mem;
		str_mem = new char[length+1];
		strncpy(str_mem, addressptr->recalc_str, length+1);
		addressptr->recalc_str = str_mem;
		print(log_debug, "Got Addressparam: %u, %u, %s", name().c_str(), addressptr->function_code, addressptr->address, addressptr->recalc_str);
		addressptr++;
	}
	
	
}

MeterModbus::~MeterModbus() {
	if(!_reset_connection){
		modbus_close(_mb);
		modbus_free(_mb);
	}
}

void MeterModbus::getHighestDigit(unsigned int number, unsigned char *digit, unsigned char *power){
	*power = 0;
	while (number!=0)
	{
		*digit = number % 10; 
		(*power)++;
		number /= 10;
	}
	(*power)--;
}

int MeterModbus::open() {
	_mb = NULL;
	_mb = modbus_new_tcp(ip(), _port);
	
	if (_mb == NULL) {
		print(log_error, "Unable to allocate libmodbus context: %s, %i", ip(), _port);
		return ERR;
	}
	print(log_debug, "Connecting to %s:%i", name().c_str(), _ip.c_str(), _port);
	if (modbus_connect(_mb) == -1) {
		print(log_error, "Connection failed: %s",name().c_str(), modbus_strerror(errno));
		modbus_free(_mb);
		return ERR;
	}
	_reset_connection = false;
	return SUCCESS;
}

int MeterModbus::close() {
	/*struct addressparam *addressptr = _addressparams;
	while(addressptr->function_code != 0xFF){
		print(log_debug, "Freeing Address %p", "", addressptr->recalc_str);
		delete[] addressptr->recalc_str;
		addressptr++;
	}
	free((void *)_addressparams);*/
	modbus_close(_mb);
	modbus_free(_mb);
	return 0;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t max_readings) {
	uint16_t in;
	double out;
	int rc;
	const struct addressparam *current_address;
	int read_count = 0;
	
	if(_reset_connection) {
		int success;
		print(log_info, "Resetting Connection to %s because of error", name().c_str(), _ip.c_str());
		rc = open();
		if(rc == SUCCESS)
			_reset_connection = false;
		else
			return 0;
	}
	current_address = _addressparams;
	unsigned char highest_digit, power;
	while((current_address->function_code != 0xFF) && (max_readings > read_count)) {
		getHighestDigit(current_address->address, &highest_digit, &power);
		switch(current_address->function_code){
			case READ_HOLDING_REGISTERS:
				print(log_debug, "Accessing Holding Register %u", name().c_str(), current_address->address);
				rc = modbus_read_registers(_mb, current_address->address-4*(unsigned int)pow((double)10,(double)power)-1, 1, &in);
				break;
			case READ_INPUT_REGISTERS:
				print(log_debug, "Accessing Input Register %u", name().c_str(), current_address->address);
				rc = modbus_read_input_registers(_mb, current_address->address-3*(unsigned int)pow((double)10,(double)power)-1, 1, &in);
				break;
			case READ_COIL_STATUS:
				print(log_debug, "Accessing Coil Status register %u", name().c_str(), current_address->address);
				rc = modbus_read_bits(_mb, current_address->address, 1, (uint8_t *)&in);
				break;
			case READ_INPUT_STATUS:
				print(log_debug, "Accessing Input Status register %u", name().c_str(), current_address->address);
				rc = modbus_read_input_bits(_mb, current_address->address-2*(unsigned int)pow((double)10,(double)power)-1, 1, (uint8_t *)&in);
				break;
		}
		
		if (rc == -1 && errno != 112345680) { //Except Illegal Data Address
			print(log_error, "Unable to fetch data (FC: %u, ADR: %u): %s", name().c_str(), current_address->function_code, current_address->address, modbus_strerror(errno));
			if(errno == 104 || errno == 32){
				close();
				_reset_connection = true;
			}
			return read_count;
		}
		
		if(rc == -1 && errno == 112345680){
			print(log_error, "Unable to fetch data (FC: %u, ADR: %u): %s", name().c_str(), current_address->function_code, current_address->address, modbus_strerror(errno));
			current_address++;
			continue;
		}
		
		print(log_debug, "Got %u via Modbus", "", in);
		// TODO ERRORS possible if wrong format string input from config file
		char *math_expression;
		asprintf(&math_expression, current_address->recalc_str, in);
		print(log_debug, "Calulating: %s --> %s", "", current_address->recalc_str, math_expression);
		out = parse_expression(math_expression);
		if(isnan(out)) {
			print(log_error, "Unable to use value read from address %u. Error calculating: %s", name().c_str(), current_address->address, math_expression);
		}
		else {
			rds[read_count].value(out);
			rds[read_count].time();
			rds[read_count].identifier(new AddressIdentifier(current_address->address));
			read_count++;
		}
		free(math_expression);
		current_address++;
	}
	
	return read_count;
}

