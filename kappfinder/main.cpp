/*
    KAppfinder, the KDE application finder

    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include "toplevel.h"

static const char description[] = I18N_NOOP( "KDE's application finder" );

static KCmdLineOptions options[] = {
  { "dir <dir>", I18N_NOOP( "Install .desktop files into directory <dir>" ), 0 },
  KCmdLineLastOption
};

int main( int argc, char *argv[] )
{
  KAboutData aboutData( "kappfinder", I18N_NOOP( "KAppfinder" ),
                        "1.0", description, KAboutData::License_GPL,
                        "(c) 1998-2000, Matthias Hoelzer-Kluepfel" );
  aboutData.addAuthor( "Matthias Hoelzer-Kluepfel", 0, "hoelzer@kde.org" );
  aboutData.addAuthor( "Tobias Koenig", 0, "tokoe@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;

  TopLevel *dlg = new TopLevel( args->getOption( "dir" ) );

  dlg->exec();
}
