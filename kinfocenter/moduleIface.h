/*
  Copyright (c) 2001 Daniel Molkentin <molkentin@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
*/

#ifndef __moduleIface_h__
#define __moduleIface_h__



#include <QFont>
#include <QPalette>
#include <QWidget>

class ModuleIface : public QObject {

Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.kde.kinfocenter")
public:
	ModuleIface(QObject *parent, const char *name);
	~ModuleIface();

public Q_SLOTS:
#if 0		
	Q_SCRIPTABLE QFont getFont();
	Q_SCRIPTABLE QPalette getPalette();
#endif	
	Q_SCRIPTABLE QString getStyle() const;
	Q_SCRIPTABLE void invokeHelp();

signals:
	void helpClicked();

private:
	QWidget *_parent;

};

#endif
