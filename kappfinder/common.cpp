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

// Own
#include "common.h"

// std
#include <stdlib.h>

// Qt
#include <QtCore/QDir>
#include <QtCore/QFile>

// KDE
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#define DBG_CODE 1213

void copyFile( const QString &inFileName, const QString &outFileName )
{
  QFile inFile( inFileName );
  if ( inFile.open( QIODevice::ReadOnly ) ) {
    QFile outFile( outFileName );
    if ( outFile.open( QIODevice::WriteOnly ) ) {
      outFile.write( inFile.readAll() );
      outFile.close();
    }

    inFile.close();
  }
}

bool scanDesktopFile( QList<AppLnkCache*> &appCache, const QString &templ,
                      QString destDir )
{
  KDesktopFile desktop( templ );

  // find out where to put the .desktop files
  QString destName;
  if ( destDir.isNull() )
    destDir = KGlobal::dirs()->saveLocation( "apps" );
  else
    destDir += '/';

  // find out the name of the file to store
  destName = templ;
  int pos = templ.indexOf( "kappfinder/apps/" );
  if ( pos > 0 )
    destName = destName.mid( pos + strlen( "kappfinder/apps/" ) );

  // calculate real dir and filename
  destName = destDir + destName;
  pos = destName.lastIndexOf( '/' );
  if ( pos > 0 ) {
    destDir = destName.left( pos );
    destName = destName.mid( pos + 1 );
  }

  // determine for which executable to look
  QString exec = desktop.desktopGroup().readPathEntry( "TryExec" );
  if ( exec.isEmpty() )
    exec = desktop.desktopGroup().readPathEntry( "Exec" );
  pos = exec.indexOf( ' ' );
  if ( pos > 0 )
    exec = exec.left( pos );

  // try to locate the binary
  QString pexec = KGlobal::dirs()->findExe( exec,
                 QString( ::getenv( "PATH" ) ) + ":/usr/X11R6/bin:/usr/games" );
  if ( pexec.isEmpty() ) {
    kDebug(DBG_CODE) << "looking for " << exec.toLocal8Bit().constData()
                      << "\t\tnot found" << endl;
    return false;
  }

  AppLnkCache *cache = new AppLnkCache;
  cache->destDir = destDir;
  cache->destName = destName;
  cache->templ = templ;
  cache->item = 0;

  appCache.append( cache );

  kDebug(DBG_CODE) << "looking for " << exec.toLocal8Bit().constData()
                    << "\t\tfound" << endl;
  return true;
}

void createDesktopFiles( QList<AppLnkCache*> &appCache, int &added )
{
  AppLnkCache* cache;
  Q_FOREACH( cache,appCache ){
    if ( cache->item == 0 || ( cache->item && cache->item->isOn() ) ) {
      added++;

      QString destDir = cache->destDir;
      QString destName = cache->destName;
      QString templ = cache->templ;

      destDir += '/';
      QDir d;
      int pos = -1;
      while ( ( pos = destDir.indexOf( '/', pos + 1 ) ) >= 0 ) {
        QString path = destDir.left( pos + 1 );
        d = path;
        if ( !d.exists() )
          d.mkdir( path );
      }

      // write out the desktop file
      copyFile( templ, destDir + '/' + destName );
    }
  }
}

void decorateDirs( QString destDir )
{
  // find out where to put the .directory files
  if ( destDir.isNull() )
    destDir = KGlobal::dirs()->saveLocation( "apps" );
  else
    destDir += '/';

  QStringList dirs = KGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.directory", KStandardDirs::Recursive );

  QStringList::Iterator it;
  for ( it = dirs.begin(); it != dirs.end(); ++it ) {
    // find out the name of the file to store
    QString destName = *it;
    int pos = destName.indexOf( "kappfinder/apps/" );
    if ( pos > 0 )
      destName = destName.mid( pos + strlen( "kappfinder/apps/" ) );

    destName = destDir + '/' + destName;

    if ( !QFile::exists( destName ) ) {
      kDebug(DBG_CODE) << "Copy " << *it << " to " << destName;
      copyFile( *it, destName );
    }
  }
}
