/*
    KAppfinder, the KDE application finder

    Copyright (c) 2002-2003 Tobias Koenig <tokoe@kde.org>

    Based on code written by Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <kcmdlineargs.h>
#include <kglobal.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <QStringList>

#include <stdio.h>

#include "common.h"


int main( int argc, char *argv[] )
{
  KComponentData componentData( "kappfinder_install" );
  int added = 0;

  if ( argc != 2 ) {
    fprintf( stderr, "Usage: kappfinder_install $directory\n" );
    return -1;
  }

  QStringList templates = KGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.desktop", KStandardDirs::Recursive );

  QString dir = QString( argv[ 1 ] ) + '/';

  QList<AppLnkCache*> appCache;

  QStringList::Iterator it;
  for ( it = templates.begin(); it != templates.end(); ++it )
    scanDesktopFile( appCache, *it, dir );

  createDesktopFiles( appCache, added );
  decorateDirs( dir );

  qDeleteAll(appCache);
  appCache.clear();

  printf( "%i application(s) added\n", added );

  return 0;
}
