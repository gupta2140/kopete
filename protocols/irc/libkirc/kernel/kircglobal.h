/*
    Kopete Export macors

    Copyright (c) 2007      by Michel Hermier <michel.hermier@gmail.com>

    Kopete    (c) 2007      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KIRCGLOBAL_H
#define KIRCGLOBAL_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace KIrc
{

typedef struct
{
	QByteArray value;
} OptArg;

static inline
KIrc::OptArg optArg(const QByteArray &arg)
{ KIrc::OptArg r; r.value = arg; return r; }

// The isNull test is intented.
static inline
QList<QByteArray> &operator << (QList<QByteArray> &list, const KIrc::OptArg &optArg)
{ if (!optArg.value.isNull()) list.append(optArg.value); return list; }

};

#endif
