#include "../stdafx.h"
#include "OscMessageStream.h"
#include "OscException.h"

OscMessageStream::OscMessageStream(const ReceivedMessage &message) : 
	suffixPos(1), msg(message), it(msg.ArgumentsBegin()), address(msg.AddressPattern())
{
	for (;suffixPos < address.length(); suffixPos++) {
		if (address[suffixPos] == '/') {
			break;
		}
	}
}

OscMessageStream::OscMessageStream(const OscMessageStream &stream) :
	suffixPos(stream.suffixPos), msg(stream.msg), it(msg.ArgumentsBegin()), address(msg.AddressPattern())
{
}

OscMessageStream::~OscMessageStream(void)
{
}

string
OscMessageStream::getAddressPattern(void) const
{
	return address;
}

string
OscMessageStream::getAddressPatternPrefix(void) const
{
	return address.substr(0, suffixPos);
}

string
OscMessageStream::getAddressPatternSuffix(void) const
{
	return address.substr(suffixPos);
}

bool
OscMessageStream::typetagMatch(const char *typetags)
{
	int len = ::strlen(typetags);
	if (msg.ArgumentCount() != len) {
		return false;
	}
	char *iterA = const_cast<char*>(typetags);
	char *iterB = const_cast<char*>(msg.TypeTags());

	for (int i = 0; i < len; i++) {
		if (*iterA != *iterB) {
			if (*iterA == 'i' && *iterB == 'f') {
				continue;
			}
			return false;
		}
		
		iterA++;
		iterB++;
	}

	return true;
}

bool
OscMessageStream::addressMatch(const char *addressPattern)
{
	return (addressPattern == address);
}

bool
OscMessageStream::endOfStream(void)
{
	return it == msg.ArgumentsEnd();
}

int
OscMessageStream::argumentCount(void)
{
	return msg.ArgumentCount();
}

void
OscMessageStream::resetStream(void)
{
	it = msg.ArgumentsBegin();
}

void
OscMessageStream::setStream(int position)
{
	it = msg.ArgumentsBegin();
	for (int i = 0; i < position; i++) {
		if (++it == msg.ArgumentsEnd()) {
			throw OscException(OscException::kOscExceptionTypeLibloError, "invalid stream position", false);
		}
	}
}

int
OscMessageStream::getInt32(void)
{
	if (it == msg.ArgumentsEnd()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "stream past endpoint - must reset first.", false);
	}

	if (it->IsInt32()) {
		return (it++)->AsInt32();
	}
	else if(it->IsFloat()) {
		return (int)( (it++)->AsFloat() );
	}
	else {
		throw OscException(OscException::kOscExceptionTypeLibloError, "invalid argument type.", false);
	}
}

float
OscMessageStream::getFloat(void)
{
	if (it == msg.ArgumentsEnd()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "stream past endpoint - must reset first.", false);
	}

	if(it->IsFloat()) {
		return (it++)->AsFloat();
	}
	else {
		throw OscException(OscException::kOscExceptionTypeLibloError, "invalid argument type.", false);
	}
}

string
OscMessageStream::getString(void)
{
	if (it == msg.ArgumentsEnd()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "stream past endpoint - must reset first.", false);
	}

	if (it->IsString()) {
		return string( (it++)->AsString() );
	}
	else {
		throw OscException(OscException::kOscExceptionTypeLibloError, "invalid argument type.", false);
	}
}
