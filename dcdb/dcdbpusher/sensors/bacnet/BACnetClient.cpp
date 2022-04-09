//================================================================================
// Name        : BACnetClient.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for BACnetClient class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#include "BACnetClient.h"

#include "bacnet/address.h"
#include "bacnet/apdu.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/datalink.h"
#include "bacnet/npdu.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/tsm.h"

double  BACnetClient::_presentValue;
uint8_t BACnetClient::_handlerTransmitBuffer[MAX_PDU];

BACnetClient::BACnetClient(const std::string &name)
    : EntityInterface(name),
      _invokeId(0),
      _timeout(1000) {
	_presentValue = 0;
	_targetAddress = {0};
}

BACnetClient::~BACnetClient() {
	datalink_cleanup();
}

void BACnetClient::init(std::string interface, const std::string &address_cache, unsigned port, unsigned timeout, unsigned apdu_timeout, unsigned retries) {
	_timeout = timeout;

	if (FILE *file = fopen(address_cache.c_str(), "r")) {
		fclose(file);
	} else {
		throw std::runtime_error("Can not open address cache file");
	}

	address_init_by_file(address_cache.c_str());

	//setup datalink

	//#if defined(BACDL_BIP)
	bip_set_port(port);
	//#endif
	apdu_timeout_set(apdu_timeout);
	apdu_retries_set(retries);

	if (!datalink_init(&interface[0])) {
		throw std::runtime_error("Failed to setup datalink");
	}
	//end setup datalink

	/* set the handler for all the services we don't implement
	   It is required to send the proper reject message... */
	apdu_set_unrecognized_service_handler_handler(unrecognizedServiceHandler);

	//NOTE: no handler for read property set even though it is required. We are no real BACnet device

	/* we only need to handle the data coming back from confirmed (read property) requests */
	apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY, readPropertyAckHandler);

	/* handle any errors coming back */
	apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, errorHandler);
	apdu_set_abort_handler(abortHandler);
	apdu_set_reject_handler(rejectHandler);
}

double BACnetClient::readProperty(uint32_t deviceObjInstance, uint32_t objInstance, BACNET_OBJECT_TYPE objType, BACNET_PROPERTY_ID objProperty, int32_t objIndex) {
	//TODO better use readPropertyMultiple? But how to assign the properties to multiple different MQTT topics?

	uint16_t pdu_lenRec = 0;
	int      pdu_lenSent = 0;
	int      bytes_sent = 0;
	unsigned max_apdu = 0;
	_targetAddress = {0};        //where message goes to
	BACNET_ADDRESS src = {0};    //where response came from
	BACNET_ADDRESS myAddr = {0}; //our address

	BACNET_READ_PROPERTY_DATA data;
	BACNET_NPDU_DATA          npdu_data;

	uint8_t RecBuf[MAX_MPDU] = {0};

	if (!address_get_by_device(deviceObjInstance, &max_apdu, &_targetAddress)) {
		throw std::runtime_error("Address not found");
	}

	/*
	if (!_targetAddress) {
		throw std::runtime_error("Destination address empty");
	}
	*/

	/*
	if (!dcc_communication_enabled()) {
		throw std::runtime_error("Communication Control disabled");
	}
	*/

	if (!(_invokeId = tsm_next_free_invokeID())) {
		throw std::runtime_error("No TSM available");
	}

	/* encode the NPDU portion of the packet */
	datalink_get_my_address(&myAddr);
	npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
	pdu_lenSent = npdu_encode_pdu(&_handlerTransmitBuffer[0], &_targetAddress, &myAddr, &npdu_data);

	/* encode the APDU portion of the packet */
	data.object_type = objType;
	data.object_instance = objInstance;
	data.object_property = objProperty;
	data.array_index = objIndex;
	pdu_lenSent += rp_encode_apdu(&_handlerTransmitBuffer[pdu_lenSent], _invokeId, &data);
	/* will it fit in the sender?
	   note: if there is a bottleneck router in between
	   us and the destination, we won't know unless
	   we have a way to check for that and update the
	   max_apdu in the address binding table. */
	if ((uint16_t)pdu_lenSent < max_apdu) {
		tsm_set_confirmed_unsegmented_transaction(_invokeId, &_targetAddress, &npdu_data, &_handlerTransmitBuffer[0], (uint16_t)pdu_lenSent);
		bytes_sent = datalink_send_pdu(&_targetAddress, &npdu_data, &_handlerTransmitBuffer[0], pdu_lenSent);
		if (bytes_sent <= 0) {
			std::string errorMsg = strerror(errno);
			std::string str = "Failed to send ReadProperty Request ";
			throw std::runtime_error(str + errorMsg);
		}
	} else {
		tsm_free_invoke_id(_invokeId);
		throw std::runtime_error("Failed to Send ReadProperty Request (exceeds destination maximum APDU)");
	}

	// returns 0 on timeout
	pdu_lenRec = datalink_receive(&src, &RecBuf[0], MAX_MPDU, _timeout);

	if (pdu_lenRec) {
		int              apdu_offset = 0;
		BACNET_ADDRESS   destRec = {0};
		BACNET_NPDU_DATA npdu_dataRec = {0};

		apdu_offset = npdu_decode(&RecBuf[0], &destRec, &src, &npdu_dataRec);
		if (npdu_data.network_layer_message) {
			/* network layer message received!  Handle it! */
			LOG(error) << "Network layer message received. Discarding";
		} else if ((apdu_offset > 0) && (apdu_offset <= pdu_lenRec)) {
			if ((destRec.net == 0) || (destRec.net == BACNET_BROADCAST_NETWORK)) {
				/* only handle the version that we know how to handle */
				/* and we are not a router, so ignore messages with
				   routing information cause they are not for us */
				if (!(destRec.net == BACNET_BROADCAST_NETWORK) && !((RecBuf[apdu_offset] & 0xF0) == PDU_TYPE_CONFIRMED_SERVICE_REQUEST)) {
					apdu_handler(&src, &RecBuf[apdu_offset], (uint16_t)(pdu_lenRec - apdu_offset));
				}
			}
		}
	} else {
		tsm_free_invoke_id(_invokeId);
		throw std::runtime_error("Timeout while waiting for response");
	}

	if (!tsm_invoke_id_free(_invokeId)) { //should be freed by apdu_handler on success
		tsm_free_invoke_id(_invokeId);
		throw std::runtime_error("Invoke ID was not freed");
	}

	return _presentValue;
}

void BACnetClient::unrecognizedServiceHandler(uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src, BACNET_CONFIRMED_SERVICE_DATA *service_data) {
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
	int                                                                       pdu_len = 0;
	int                                                                       bytes_sent = 0;
	BACNET_NPDU_DATA                                                          npdu_data;
	BACNET_ADDRESS                                                            myAddress;

	(void)service_request;
	(void)service_len;

	/* encode the NPDU portion of the packet */
	datalink_get_my_address(&myAddress);
	npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
	pdu_len = npdu_encode_pdu(&_handlerTransmitBuffer[0], src, &myAddress, &npdu_data);
	/* encode the APDU portion of the packet */
	pdu_len += reject_encode_apdu(&_handlerTransmitBuffer[pdu_len], service_data->invoke_id, REJECT_REASON_UNRECOGNIZED_SERVICE);
	/* send the data */
	bytes_sent = datalink_send_pdu(src, &npdu_data, &_handlerTransmitBuffer[0], pdu_len);
	if (bytes_sent > 0) {
		LOG(info) << "BACnet: Sent Reject";
	} else {
		LOG(info) << "BACnet: Could not send Reject: " << strerror(errno);
	}
}

void BACnetClient::readPropertyAckHandler(uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src, BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data) {
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
	int                                                                       serviceLen = 0;
	//int appDataLen = 0;
	uint8_t *                     application_data;
	int                           application_data_len;
	BACNET_READ_PROPERTY_DATA     data;
	BACNET_APPLICATION_DATA_VALUE value; /* for decode value data */

	/*if (!(address_match(&_targetAddress, src) && (service_data->invoke_id == _invokeId))) {
    	throw std::runtime_error("Message not determined for us");
    }*/

	serviceLen = rp_ack_decode_service_request(service_request, service_len, &data);
	if (serviceLen <= 0) {
		tsm_free_invoke_id(service_data->invoke_id);
		throw std::runtime_error("Decode failed");
	}
	//     rp_ack_print_data(&data);

	application_data = data.application_data;
	application_data_len = data.application_data_len;
	/*appDataLen = */ bacapp_decode_application_data(application_data, (uint8_t)application_data_len, &value);

	//bacapp_print_value(stdout, &object_value);

	//TODO what kind of data is returned? which cases do we need to handle? fit they into int64_t at all??
	switch (value.tag) {
		case BACNET_APPLICATION_TAG_NULL:
			LOG(trace) << "TAG_NULL";
			_presentValue = 0;
			break;
		case BACNET_APPLICATION_TAG_BOOLEAN:
			LOG(trace) << "TAG_BOOLEAN";
			_presentValue = (value.type.Boolean) ? 1 : 0;
			break;
		case BACNET_APPLICATION_TAG_UNSIGNED_INT:
			LOG(trace) << "TAG_UNSIGNED_INT";
			_presentValue = (unsigned long)value.type.Unsigned_Int;
			break;
		case BACNET_APPLICATION_TAG_SIGNED_INT:
			LOG(trace) << "TAG_SIGNED_INT";
			_presentValue = (long)value.type.Signed_Int;
			break;
		case BACNET_APPLICATION_TAG_REAL:
			LOG(trace) << "TAG_REAL";
			_presentValue = (double)value.type.Real;
			break;
		case BACNET_APPLICATION_TAG_DOUBLE:
			LOG(trace) << "TAG_DOUBLE";
			_presentValue = value.type.Double;
			break;
		default:
			tsm_free_invoke_id(service_data->invoke_id);
			throw std::runtime_error("Value tag not supported");
			break;
	}
}

void BACnetClient::errorHandler(BACNET_ADDRESS *src, uint8_t invokeId, BACNET_ERROR_CLASS error_class, BACNET_ERROR_CODE error_code) {
	tsm_free_invoke_id(invokeId);
	std::string str = "BACnet Error: ";
	std::string errorMsg1 = bactext_error_class_name((int)error_class);
	std::string errorMsg2 = bactext_error_code_name((int)error_code);
	throw std::runtime_error(str + errorMsg1 + errorMsg2);
}

void BACnetClient::abortHandler(BACNET_ADDRESS *src, uint8_t invokeId, uint8_t abort_reason, bool server) {
	tsm_free_invoke_id(invokeId);
	std::string str = "BACnet Abort: ";
	std::string errorMsg = bactext_abort_reason_name((int)abort_reason);
	throw std::runtime_error(str + errorMsg);
}

void BACnetClient::rejectHandler(BACNET_ADDRESS *src, uint8_t invokeId, uint8_t reject_reason) {
	tsm_free_invoke_id(invokeId);
	std::string str = "BACnet Reject: ";
	std::string errorMsg = bactext_reject_reason_name((int)reject_reason);
	throw std::runtime_error(str + errorMsg);
}

void BACnetClient::printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "Timeout: " << _timeout;
}
