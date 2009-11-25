#ifndef JINGLE_RTP_MANAGER
#define JINGLE_RTP_MANAGER

#include <QObject>
#include <ortp/ortp.h>

class QByteArray;
class QAbstractSocket;
class QUdpSocket;
class QDomElement;

//class MediaSession;
//class JingleRtpSession;

class RtpPacket
{
public :
	RtpPacket();
	RtpPacket(mblk_t *msg);
	RtpPacket(const QByteArray& data);
	~RtpPacket();

	enum RtpType
	{
		Rtp = 0,
		Rtcp,
		Unknown
	};
	
	char* constData() const;
	QByteArray data();
	size_t size();
	void setType(RtpType type);
	RtpType type() const;

private:
	char* m_data;
	size_t m_size;
	RtpType m_type;

};

class JingleRtpSession;

class JingleRtpManager : public QObject
{
	Q_OBJECT
	friend class JingleRtpSession;
public:
	JingleRtpManager();
	~JingleRtpManager();

	void appendSession(JingleRtpSession*);
	static JingleRtpManager* manager();

	/*
	 * Set media data which will be packed and sent when needed by sessions.
	 */
	void setMediaData(const QByteArray&);

private:
	QList<JingleRtpSession*> m_sessions;
	QByteArray m_mediaDataIn;	
	JingleRtpSession* sessionWithTransport(RtpTransport*);
};


class JingleRtpSession : public QObject
{
	Q_OBJECT
	friend class JingleRtpManager;
public:
	/*
	 * Directions :
	 * 	In = for data coming in.
	 * 	Out = for data going out.
	 */
	enum Direction {In = 0, Out};
	
	/*
	 * Creates a new RTP session with direction dir.
	 */
	JingleRtpSession();

	/*
	 * Destroys the RTP sessions and frees all allocated memory.
	 */
	~JingleRtpSession();
	
	/*
	 * Sets the payload type used for this session.
	 * The argument is the payload type in a payload-type XML tag.
	 */
	void setPayload(const QDomElement& payload); //FIXME:should not be a QDomElement anymore.

	/**
	 * Packs the data into an RTP packet ready to be sent.
	 *
	 * Once the packet is packed or unpacked, the signal packed() or
	 * unpacked() will be emitted respectively.
	 */
	void pack(const QByteArray&, int ts);
	void unpack(const QByteArray&);

	RtpTransport* rtpTransport() const;
	RtpTransport* rtcpTransport() const;

private slots:
	/*
	 * Called when rtp or rtcp data is ready to be read and sent on the network.
	 * The argument is an integer giving the channel on which a packet ir ready :
	 * 	1 : Rtp
	 * 	2 : Rtcp
	 */
	//void readyRead(int);
	//void rtcpDataReady(); // Maybe not used.

public slots:
	QByteArray getPacked();
	QByteArray getUnpacked(int ts);
	
signals:
	//void dataSent();
	//void packetOutReady(RtpPacket*);

	void packedReady();
	void unpackedReady();

private:
	RtpSession *m_rtpSession;

	RtpTransport m_rtpTransport;
	RtpTransport m_rtcpTransport;

	int payloadID;
	QString payloadName;
	enum State {SendingData = 0} state;
	Direction m_direction;
	int bufSize;
	QByteArray inData;

	//FIXME:Is there a better class for queues ?
	QList<QByteArray*> rtpPacketsOut;
	QList<RtpPacket*> rtpPacketsIn;

	RtpSession* rtpSession() const;

	void appendRtpPacketOut(mblk_t *msg);

	static int sendRtpTo(RtpTransport *t, mblk_t *msg, int flags, const struct sockaddr *to, socklen_t tolen);
	static int recvRtpFrom(RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
	static int sendRtcpTo(RtpTransport *t, mblk_t *msg, int flags, const struct sockaddr *to, socklen_t tolen);
	static int recvRtcpFrom(RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
	
	void fillRtpPacket(mblk_t*);
	void appendRtpPacketOut(RtpPacket*);

	RtpPacket *rtpPacketOut;
};

#endif //JINGLE_RTP_MANAGER