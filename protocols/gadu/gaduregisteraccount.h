// -*- Mode: c++-mode; c-basic-offset: 2; indent-tabs-mode: t; tab-width: 2; -*-
//
// Copyright (C) 2003 Grzegorz Jaskiewicz 	<gj at pointblue.com.pl>
//
// gaduregisteraccount.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef GADUREGISTERACCOUNT_H
#define GADUREGISTERACCOUNT_H

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qregexp.h>

#include <kdialogbase.h>
#include <ktextedit.h>
#include <klocale.h>
#include <klineedit.h>

#include "gaduregisteraccountui.h"
#include "gaducommands.h"

class GaduRegisterAccount : public KDialogBase
{
    Q_OBJECT

public:
	GaduRegisterAccount( QWidget* , const char* );
	~GaduRegisterAccount( );

signals:
	void registeredNumber( unsigned int, QString  );

protected slots:
	void slotApply();
	void displayToken( QPixmap, QString );
	void registrationError(  const QString&, const QString& );
	void registrationDone(  const QString&,  const QString& );
	void emailChanged( const QString & );
	void passwordsChanged( const QString & );
	void doRegister();
	void tokenChanged( const QString & );
	void updateStatus( const QString status );

private:
	void validateInput();

	GaduRegisterAccountUI*	ui;
	RegisterCommand*		cRegister;
	QRegExp*				emailRegexp; 
};

#endif
