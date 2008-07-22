#include "jinglecontent.h"

//----------------------
// JingleContent
//----------------------

using namespace XMPP;

class JingleContent::Private
{
public:
	QList<QDomElement> payloads;
	QDomElement transport;
	QList<QDomElement> candidates;
	QString creator;
	QString name;
	QString profile;
	QString descriptionNS;
	//The application will access this socket directly, Iris has not to deal with RTP.
	QUdpSocket *socket; //Currently, this is the raw-udp socket for this content.
	bool sending;
	bool receiving;
};

JingleContent::JingleContent()
: d(new Private())
{
	d->sending = false;
	d->receiving = false;
	d->socket = 0L;
}

JingleContent::~JingleContent()
{

}

void JingleContent::addPayloadType(const QDomElement& pl)
{
	d->payloads << pl;
}

void JingleContent::addPayloadTypes(const QList<QDomElement>& pl)
{
	d->payloads << pl;
}

void JingleContent::setTransport(const QDomElement& t)
{
	d->transport = t;
}

QList<QDomElement> JingleContent::payloadTypes() const
{
	return d->payloads;
}

QDomElement JingleContent::transport() const
{
	return d->transport;
}

void JingleContent::setCreator(const QString& c)
{
	d->creator = c;
}

void JingleContent::setName(const QString& n)
{
	d->name = n;
}

void JingleContent::setDescriptionNS(const QString& desc)
{
	d->descriptionNS = desc;
}

void JingleContent::setProfile(const QString& p)
{
	d->profile = p;
}

void JingleContent::fromElement(const QDomElement& e)
{
	// FIXME:tag order may not always be the same !!!
	if (e.tagName() != "content")
		return;
	d->creator = e.attribute("creator");
	d->name = e.attribute("name");
	//d->sender = e.attribute("sender");
	QDomElement desc = e.firstChildElement();
	d->descriptionNS = desc.attribute("xmlns");
	d->profile = desc.attribute("profile");
	QDomElement payload = desc.firstChildElement();
	while (!payload.isNull())
	{
		d->payloads << payload;
		payload = payload.nextSiblingElement();
	}
	QDomElement transport = desc.nextSiblingElement();
	d->transport = transport;
}

QDomElement JingleContent::contentElement()
{
	// Create the QDomElement which has to be returned.
	QDomDocument doc("");
	
	QDomElement content = doc.createElement("content");
	content.setAttribute("creator", d->creator);
	content.setAttribute("name", d->name);
	content.setAttribute("sender", "both"); //Setting to default currently, change it !
	
	QDomElement description = doc.createElement("description");
	description.setAttribute("xmlns", d->descriptionNS);
	description.setAttribute("profile", d->profile);

	for (int i = 0; i < d->payloads.count(); i++)
	{
		description.appendChild(d->payloads.at(i));
	}
	content.appendChild(description);
	content.appendChild(d->transport);

	return content;
}

QString JingleContent::name() const
{
	return d->name;
}

QString JingleContent::descriptionNS() const
{
	return d->descriptionNS;
}

JingleContent::Type JingleContent::dataType()
{
	if (d->descriptionNS == "urn:xmpp:tmp:jingle:apps:audio-rtp")
		return Audio;
	else if (d->descriptionNS == "urn:xmpp:tmp:jingle:apps:video-rtp")
		return Video;
	else if (d->descriptionNS == "urn:xmpp:tmp:jingle:apps:file-transfer")
		return FileTransfer;
	else
		return Unknown;
}

void JingleContent::addTransportInfo(const QDomElement& e)
{
	QDomElement transport = e.firstChildElement();
	if (transport.attribute("xmlns") == "urn:xmpp:tmp:jingle:transports:ice-udp")
	{
		if (d->transport.attribute("pwd") != transport.attribute("pwd"))
		{
			qDebug() << "Bad ICE Password !";
			return;
		}
		
		if (d->transport.attribute("ufrag") != transport.attribute("ufrag"))
		{
			qDebug() << "Bad ICE User Fragment !";
			return;
		}
		QDomElement child = transport.firstChildElement();
		//FIXME:Is it possible to have more than one candidate per transport-info ?
		//	See Thread "Jingle: multiple candidates per transport-info?" on xmpp-standards.
		if (child.tagName() == "candidate")
		{
			// Just adding the Xml Element.
			d->candidates << child;
		}
	}
}

QString JingleContent::iceUdpPassword()
{
	if (d->transport.attribute("xmlns") == "urn:xmpp:tmp:jingle:transports:ice-udp")
		return d->transport.attribute("pwd");
	return "";
}

QString JingleContent::iceUdpUFrag()
{
	if (d->transport.attribute("xmlns") == "urn:xmpp:tmp:jingle:transports:ice-udp")
		return d->transport.attribute("ufrag");
	return "";
}

void JingleContent::createUdpInSocket()
{
	if (d->transport.attribute("xmlns") != "urn:xmpp:tmp:jingle:transports:raw-udp")
		return;
	qDebug() << "JingleContent::createUdpInSocket()";
	if (!d->socket)
		d->socket = new QUdpSocket();
	d->socket->bind(QHostAddress(d->transport.firstChildElement().attribute("ip")), d->transport.firstChildElement().attribute("port").toInt());
	connect(d->socket, SIGNAL(readyRead()), this, SIGNAL(rawUdpDataReady()));
}

QUdpSocket *JingleContent::socket()
{
	qDebug() << "Getting socket from content" << name();
	return d->socket;
}

bool JingleContent::sending()
{
	return d->sending;
}

void JingleContent::setSending(bool s)
{
	d->sending = s;
}

bool JingleContent::receiving()
{
	return d->receiving;
}

void JingleContent::setReceiving(bool r)
{
	d->receiving = r;
}

void JingleContent::startSending()
{
	//Create udp OUT socket
	if (!d->socket)
		d->socket = new QUdpSocket();
	d->socket->connectToHost(d->transport.firstChildElement().attribute("ip"), d->transport.firstChildElement().attribute("port").toInt());
	QHostAddress address;
	address.setAddress(transport().firstChildElement().attribute("ip"));
	qDebug() << "Sending data to" << address.toString() << ":" << transport().firstChildElement().attribute("port").toInt();
	qDebug() << "EMIT needData(c) SIGNAL";
	emit needData(this);
}

QList<QDomElement> JingleContent::candidates() const
{
	return d->candidates;
}

QString JingleContent::creator() const
{
	return d->creator;
}

QString JingleContent::profile() const
{
	return d->profile;
}

JingleContent& JingleContent::operator=(const JingleContent &other)
{
	d->payloads = other.payloadTypes();
	d->transport = other.transport();
	d->candidates = other.candidates();
	d->creator = other.creator();
	d->name = other.name();
	d->profile = other.profile();
	d->descriptionNS = other.descriptionNS();
	
	return *this;
}
