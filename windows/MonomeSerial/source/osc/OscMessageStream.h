#ifndef __OSCMESSAGESTREAM_H__
#define __OSCMESSAGESTREAM_H__

#include "oscpack/OscReceivedElements.h"
#include <string>

using namespace std;
using namespace osc;

class OscMessageStream
{
public:
	OscMessageStream(const ReceivedMessage &message);
	OscMessageStream(const OscMessageStream &stream);

	~OscMessageStream(void);

	string getAddressPattern(void) const;
	string getAddressPatternPrefix(void) const;
	string getAddressPatternSuffix(void) const;

	bool typetagMatch(const char *typetags);
	bool addressMatch(const char *addressPattern);
	bool endOfStream(void);

	int argumentCount(void);

	void resetStream(void);
	void setStream(int position);

	int getInt32(void);
	float getFloat(void);
	string getString(void);

private:
	ReceivedMessage msg;
	string address;
	int suffixPos;
	ReceivedMessageArgumentIterator it;
};

#endif //__OSCMESSAGESTREAM_H__