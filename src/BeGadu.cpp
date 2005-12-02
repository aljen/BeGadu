/*
	BeGadu.cpp - Client G*adu-G*adu for BeOS
	Code: aljen <aljen@gumisie.org>
	Homepage: http://gadu.beos.pl
*/

#include <Deskbar.h>
#include <Path.h>
#include <Roster.h>
#include <StorageKit.h>
#include <File.h>
#include <Screen.h>
#include <String.h>
#include <Alert.h>

#include "Msg.h"
#include "BeGadu.h"
#include "Network.h"
#include "Main.h"
#include "Person.h"
#include "Debug.h"
#include "Preferences.h"
#include "ProfileWizard.h"

#ifdef ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

#ifdef DEBUG
#define DEBUG_TRACE(str) fprintf(stderr, str)
#else
#define DEBUG_TRACE(str)
#endif

BeGadu::BeGadu() : BApplication( "application/x-vnd.BeGadu" )
	{
	/* we're checking configuration */
	iFirstRun = false;
	iHideAtStart = false;
	iLastProfile = new BString();
	iLastProfile->SetTo( "" );
	iReadyToRun = false;
	BPath path;
	BEntry entry;
	status_t error;
	
#ifdef ZETA
	app_info appinfo;
	GetAppInfo( &appinfo );
	BPath localization;
	BEntry entryloc( &appinfo.ref, true );
	entryloc.GetPath( &localization );
	localization.GetParent( &localization );
	localization.Append( "Language/Dictionaries/BeGadu" );
	BString localization_string;
	if( localization.InitCheck() != B_OK )
		localization_string.SetTo( "Language/Dictionaries/BeGadu" );
	else
		localization_string.SetTo( localization.Path() );
	be_locale.LoadLanguageFile( localization_string.String() );
#endif
	
	find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	BDirectory * bg_conf = new BDirectory( path.Path() );
	
	if( bg_conf->FindEntry( "BeGadu", &entry ) != B_OK )
		{
		path.Append( "BeGadu" );
		bg_conf->CreateDirectory( path.Path(), bg_conf );
		}
	
	if( bg_conf->FindEntry( "Profiles", &entry ) != B_OK )
		{
		find_directory( B_USER_SETTINGS_DIRECTORY, &path );
		path.Append( "BeGadu/Profiles" );
		error = bg_conf->CreateDirectory( path.Path(), bg_conf );
		iFirstRun = true;
		}
	
	find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	path.Append( "BeGadu/Profiles" );
	bg_conf->SetTo( path.Path() );
	
	if( bg_conf->CountEntries() == 0 )
		{
		iFirstRun = true;
		}
	else
		{
		iFirstRun = false;	
		}
	
	/* loading configuration */
	find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	path.Append( "BeGadu/Config" );
	BFile file( path.Path(), B_READ_ONLY );
	fprintf( stderr, _T("Loading configuration from %s\n"), path.Path() );
	BMessage *cfgmesg = new BMessage();
	if( file.InitCheck() == B_OK )
		{
		cfgmesg->Unflatten( &file );
		file.Unset();
		}
	if( cfgmesg->FindBool( "iFirstRun", &iFirstRun ) != B_OK )
		{
		iFirstRun =  true;
		}
	if( cfgmesg->FindString( "iLastProfile", iLastProfile ) != B_OK )
		{
		iLastProfile->SetTo( "" );
		}
		
	delete cfgmesg;
	delete bg_conf;
	
	if( iFirstRun )
		{
		fprintf( stderr, _T("First run, starting Profile Wizard...\n") );
		BMessenger( this ).SendMessage( new BMessage( OPEN_PROFILE_WIZARD ) );
		}
	else
		{
		fprintf( stderr, _T("Configuration OK.\n") );
		BMessenger( this ).SendMessage( new BMessage( CONFIG_OK ) );
		}
	}

void BeGadu::MessageReceived( BMessage *aMessage )
	{
	switch( aMessage->what )
		{
		/* sending mesgs from libgadu to network */
		case GOT_MESSAGE:
		case ADD_HANDLER:
		case DEL_HANDLER:
			{
			BMessenger( iWindow->GetNetwork() ).SendMessage( aMessage );
			break;
			}
		
		case ADD_MESSENGER:
			{
			DEBUG_TRACE( "BeGadu::MessageReceived( ADD_MESSENGER )\n" );
			BMessenger messenger;
			aMessage->FindMessenger( "messenger", &messenger );
			if( iWindow )
				{
				iWindow->SetMessenger( messenger );
				}
			break;
			}
			
		case SET_AVAIL:
		case SET_BRB:
		case SET_INVIS:
		case SET_NOT_AVAIL:
		case SET_DESCRIPTION:
		case BEGG_ABOUT:
		case SHOW_MAIN_WINDOW:
		case CHANGE_DESCRIPTION:
			{
			if( iWindow )
				{
				BMessenger( iWindow ).SendMessage( aMessage );
				}
			break;
			}
		
		case OPEN_PROFILE_WIZARD:
			{
			ProfileWizard *pw = new ProfileWizard();
			pw->Show();
			break;
			}

		case CONFIG_OK:
			{
			iReadyToRun = true;
			AddDeskbarIcon();
			iWindow = new MainWindow( iLastProfile );
			if( !iHideAtStart )
				{
				if( iWindow->LockLooper() )
					{
					iWindow->Show();
					iWindow->UnlockLooper();
					}
				}
			else
				{
				if( iWindow->LockLooper() )
					{
					iWindow->Show();
					iWindow->Hide();
					iWindow->UnlockLooper();
					}
				}
			break;
			}
		
		case PROFILE_CREATED:
			{
			iReadyToRun = true;
			AddDeskbarIcon();
			aMessage->FindString( "ProfileName", iLastProfile );
			fprintf( stderr, _T("Setting last profile to %s\n"), iLastProfile->String() );
			iFirstRun = false;
			iWindow = new MainWindow( iLastProfile );
			if( iWindow->LockLooper() )
				{
				if( iWindow->IsHidden() )
					{
					iWindow->Show();
					}
				else
					{
					iWindow->Activate();
					}
				iWindow->UnlockLooper();
				}
			break;
			}

		case BEGG_QUIT:
			{
			BMessenger( iWindow ).SendMessage( aMessage );
			break;
			}

		default:
			{
			BApplication::MessageReceived( aMessage );
			break;
			}
		}
	}

void BeGadu::ReadyToRun()
	{
	DEBUG_TRACE( "BeGadu::ReadyToRun()\n" );
	if( iReadyToRun )
		AddDeskbarIcon();
	}

bool BeGadu::QuitRequested()
	{
	DelDeskbarIcon();
	if( iWindow->Lock() )
		{
		iWindow->Quit();
		}
		
	/* saving configuration */
	BMessage * cfgmesg = new BMessage();
	cfgmesg->AddBool( "iFirstRun", iFirstRun );
	cfgmesg->AddBool( "iHideAtStart", iHideAtStart );
	cfgmesg->AddString( "iLastProfile", *iLastProfile );
	
	BPath path;
	find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	path.Append( "BeGadu/Config" );
	fprintf( stderr, _T("Last profile %s\n"), iLastProfile->String() );
	fprintf( stderr, _T("Saving configuration to %s\n"), path.Path() );
	BFile file( path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
	if( file.InitCheck() == B_OK )
		{
		cfgmesg->Flatten( &file );
		file.Unset();
		}
	
	delete cfgmesg;
	BApplication::QuitRequested();
	return true;
	}

void BeGadu::AddDeskbarIcon()
	{
	DEBUG_TRACE( "BeGadu::AddDeskbarIcon()\n" );
	BDeskbar deskbar;
	if( !deskbar.HasItem( "BGDeskbar" ) )
		{
		BRoster roster;
		entry_ref ref;
		status_t status = roster.FindApp( "application/x-vnd.BeGadu", &ref );
		if( status != B_OK )
			{
			fprintf( stderr, _T("Can't find BeGadu running: %s\n"), strerror( status ) );
			return;
			}
		status = deskbar.AddItem( &ref );
		if( status != B_OK )
			{
			fprintf( stderr, _T("Can't put BeGadu into Deskbar: %s\n"), strerror( status ) );
			return;
			}
		}
	}

void BeGadu::DelDeskbarIcon()
	{
	DEBUG_TRACE( "BeGadu::DelDeskbarIcon()\n" );
	BDeskbar deskbar;
	if( deskbar.HasItem( "BGDeskbar" ) )
		{
		deskbar.RemoveItem( "BGDeskbar" );
		}
	}

MainWindow* BeGadu::GetMainWindow() const
	{
	return iWindow;
	}
	
DebugWindow* BeGadu::GetDebugWindow() const
	{
	return iDebugWindow;
	}

bool BeGadu::FirstRun()
	{
	return iFirstRun;
	}

bool BeGadu::HideAtStart()
	{
	return iHideAtStart;
	}

BString* BeGadu::LastProfile()
	{
	return iLastProfile;
	}

int main()
{
	BeGadu	*bgg = new BeGadu();
	bgg->Run();
	delete bgg;
	return 0;
}
