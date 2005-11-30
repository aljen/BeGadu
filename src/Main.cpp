/**
 *	Main.cpp
 *	Code: aljen <aljen@gumisie.be>
 *	Homepage: http://begadu.sf.net
 */

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

#include "GaduListItem.h"
#include "GaduMenuItem.h"
#include "GfxStuff.h"
#include "Main.h"
#include "Msg.h"
#include "Network.h"
#include "Preferences.h"
#include "Person.h"

#define MAINWINDOW_RECT BRect(50,50,300,350)
#define MAINWINDOW_NAME	"BeGadu"

MainWindow::MainWindow( BString *aProfile )
 	: BWindow( MAINWINDOW_RECT,
 			   MAINWINDOW_NAME,
 			   B_TITLED_WINDOW,
 			   B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS )
	{
	fprintf( stderr, "MainWindow::MainWindow()\n" );
	iProfile = new Profile();
	iProfile->Load( aProfile );
	SetTitle( aProfile->String() );
	BRoster roster;
	entry_ref ref;
	BFile resfile;
	roster.FindApp( "application/x-vnd.BeGadu", &ref );
	resfile.SetTo( &ref, B_READ_ONLY );
	iResources.SetTo( &resfile );
	LoadIcons();

	SetSizeLimits( 250, 600, 300, 600 );
	iNetwork = new Network( iProfile, iProfile->GetUserlist()->GetList() );
	iNetwork->GotWindow( this );
	iListItems = new List( 512 );
	if( iProfile->GetRect().IsValid() )
		MoveTo( iProfile->GetRect().left, iProfile->GetRect().top );

	/* setup menu */
	BRect r = Bounds();
	BMenuBar *menuBar = new BMenuBar( r, "menu bar" );
	BMenu *menu;
	menu = new BMenu( "Menu" );
	iSubMenu = new BMenu( "Profile" );
	BMenuItem *item = new BMenuItem( "Siakis profil", new BMessage() );
	iSubMenu->AddItem( item );
	item = new BMenuItem( "Siakis profil inny", new BMessage() );
	iSubMenu->AddItem( item );
	iProfileItem = new BMenuItem( iSubMenu, NULL );
	menu->AddItem( iProfileItem );
	menu->AddSeparatorItem();

	iSubMenu = new BMenu( "Lista" );
	iListImport = new BMenuItem( "Import", new BMessage( BEGG_IMPORT_LIST ) );
	iListExport = new BMenuItem( "Eksport", new BMessage() );
	iSubMenu->AddItem( iListImport );
	iSubMenu->AddItem( iListExport );
	iListMenu = new BMenuItem( iSubMenu, NULL );
	menu->AddItem( iListMenu );
	menu->AddSeparatorItem();

	iPreferences = new BMenuItem( "Ustawienia", new BMessage( BEGG_PREFERENCES ) );
	menu->AddItem( iPreferences );
	iAbout = new BMenuItem( "O BeGadu..", new BMessage( BEGG_ABOUT ) );
	menu->AddItem( iAbout );
	menu->AddItem( new BMenuItem( "Zakończ", new BMessage( B_QUIT_REQUESTED ) ) );
	menuBar->AddItem( menu );
	AddChild( menuBar );

	r.top = menuBar->Bounds().bottom ;
//	r.bottom = r.top + 32;
//	fIconsView = new BView( BRect(r.left, r.top, r.right, r.bottom), "IconsView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP , B_FULL_UPDATE_ON_RESIZE);
//	fIconsView->SetViewColor(100,100,100);
//	AddChild(fIconsView);

//	BBitmap *stan1 = LoadGFX("/boot/apps/BeGadu/images/1.png");
//	BBitmap *stan2 = LoadGFX("/boot/apps/BeGadu/images/2.png");
//	IconControl *iconcontrol = new IconControl(BRect(r.left, r.top + 3, r.left + 30, r.top + 30), "iconcontrol", stan1, stan2, new BMessage(B_QUIT_REQUESTED));
//	AddChild(iconcontrol);
	
//	r = Bounds();
//	r.top = fIconsView->Bounds().bottom + menuBar->Bounds().bottom;
	
	iGaduView = new BView( r,
						   "iGaduView",
						   B_FOLLOW_ALL,
						   B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE );
	iGaduView->SetViewColor( 90, 90, 90 );
	AddChild( iGaduView );
	
	/* list */
	r = iGaduView->Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= 30;

	iListView = new BListView( r,
							   "iListView",
							   B_SINGLE_SELECTION_LIST,
							   B_FOLLOW_ALL_SIDES,
							   B_WILL_DRAW | B_FRAME_EVENTS |
							   B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE );
	iListView->SetViewColor( 110, 110, 110 );
	BFont *font = new BFont( be_plain_font );
	font->SetSize( 15.0 );
	font->SetEncoding( B_ISO_8859_2 );
	iListView->SetFont( font );
	iScrollView = new BScrollView( "iScrollView",
								   iListView,
								   B_FOLLOW_ALL,
								   B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE,
								   false,
								   true,
								   B_PLAIN_BORDER );
	iGaduView->AddChild( iScrollView );
	
	/* we must know, if user selects/opens a person */
	iListView->SetSelectionMessage( new BMessage( BEGG_PERSON_SELECTED ) );
	iListView->SetInvocationMessage( new BMessage( BEGG_PERSON_OPEN ) );

	/* adding status :) */
	r = iGaduView->Bounds();
	r.top = iListView->Bounds().bottom + 5;
//	r.top = iGaduView->Bounds().bottom;
	r.left = r.left + 5;
	r.right = r.right -5;

	BMenuItem *selectstatus;
	iStatusMenu = new BPopUpMenu( "change_status" );
	iAvail = new GaduMenuItem( iIconAvail, "Dostępny", new BMessage( SET_AVAIL ) );
	iBrb = new GaduMenuItem( iIconBrb, "Zaraz wracam", new BMessage( SET_BRB ) );
	iInvis = new GaduMenuItem( iIconInvis, "Niewidoczny", new BMessage( SET_INVIS ) );
	iNotAvail = new GaduMenuItem( iIconNotAvail, "Niedostępny", new BMessage( SET_NOT_AVAIL ) );
	iDescr = new GaduMenuItem( iIconAvailDescr, "Z Opisem", new BMessage( SET_DESCRIPTION ) );
	
	iStatusMenu->AddItem( iAvail );
	iStatusMenu->AddItem( iBrb );
	iStatusMenu->AddItem( iInvis );
	iStatusMenu->AddItem( iNotAvail);
	iStatusMenu->AddItem( iDescr);
	iNotAvail->SetMarked( true );
	
	iStatus = new BMenuField( r,
							  "iStatus",
							  "Status:",
							  iStatusMenu,
							  B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW |
							  B_NAVIGABLE | B_FRAME_EVENTS );
	iGaduView->AddChild( iStatus );

	// FIXME: to check
	// add_system_beep_event( ONLINE_SOUND );
	// add_system_beep_event( MESSAGE_SOUND );
	
	if( iProfile->GetUserlist()->IsValid() )
		{
		iListItems = iProfile->GetUserlist()->GetList();
		BMessenger( this ).SendMessage( BEGG_UPDATE_LIST );
		}

	fprintf( stderr,"Profile %s loaded.\n", iProfile->GetProfileName()->String() );

	if( iProfile()->GetAutoStatus() != GG_STATUS_NOT_AVAIL )
		{
		if( iNetwork->GetSession() )
			gg_change_status( iNetwork->GetSession(), iProfile->GetAutoStatus() );
		else
			iNetwork->Login( iProfile->GetAutoStatus() );
		}
	}

bool MainWindow::QuitRequested()
	{
	fprintf( stderr, "MainWindow::QuitRequested()\n" );

	/* saving profile settings */
	iProfile->SetRect( Frame() );
	iProfile->Save();

	/* cleaning up ;D */
	iNetwork->GotWindow( NULL );
	iNetwork->Quit();
	BMessenger( be_app ).SendMessage( B_QUIT_REQUESTED );		
	return true;
	}

void MainWindow::MessageReceived( BMessage *aMessage )
	{
	switch( aMessage->what )
		{
		case BEGG_PREFERENCES:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PREFERENCES )\n" );
			BScreen *screen = new BScreen( this );
			display_mode mode;
			screen->GetMode( &mode );
			int32 width = 600;
			int32 heigth = 400; 
			int32 x_wind = tryb.timing.h_display / 2 - ( width / 2 );
			int32 y_wind = tryb.timing.v_display / 2 - ( heigth / 2 );
			int32 new_width = x_wind + width;	// x 2
			int32 new_heigth = y_wind + heigth;		// x 2
			Preferences	*prefs;
			prefs = new Preferences( iProfile,
									 this,
									 BRect( x_wind, y_wind, new_width, new_heigth),
									 &iResources );
			prefs->Show();
			break;
			}

		case BEGG_ABOUT:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_ABOUT )\n" );
			// AboutWindow *about;
			// about = new AboutWindow();
			// about->Show();
			break;
			}

		case BEGG_PERSON_SELECTED:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PERSON_SELECTED )\n" );
			if( iListView->CurrentSelection() < 0 )
				break;
			Person *person = NULL;
			GaduListItem *who = ( GaduListItem* ) iListView->ItemAt( iListView->CurrentSelection() );
			for( int i = 0; i < iProfil->GetUserlist()->GetList()->CountItems(); i++ )
				{
				person = ( Person* ) iProfile->GetUserlist()->GetList()->ItemAt( i );
				if( !person->GetDisplay()->Compare( who->GetName().String() ) )
					break;
				}
			break;
			}
		
		case BEGG_PERSON_OPEN:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_PERSON_OPEN )\n" );
			if( iListView->CurrentSelection() < 0 )
				break;
			Person *person = NULL;
			GaduListItem *who = ( GaduListItem* ) iListView->ItemAt( iListView->CurrentSelection());
			for( int i = 0; i < iProfile->GetUserlist()->GetList()->CountItems(); i++ )
				{
				person = ( Person* ) iProfile->GetUserlist()->iList()->ItemAt( i );
				if( !person->GetDisplay()->Compare( who->GetName().String() ) )
					break;
				}
				
			if( person->GetUIN() == iProfil()->GetUIN() )
				break;
				
			BMessage *message;
			message = new BMessage( OPEN_MESSAGE );
			message->AddInt32( "who", person->GetUIN() );
			BMessenger( iNetwork ).SendMessage( message );		
			delete message;
			break;
			}

		case SET_AVAIL:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_AVAIL )\n" );
			if( iNetwork->GetSession() )
				{
				gg_change_status( iNetwork->GetSession(), GG_STATUS_AVAIL );
				iNetwork->SetStatus( GG_STATUS_AVAIL );
				BMessenger( this ).SendMessage( BEGG_UPDATE_LIST );
				}
			else
				iNetwork->Login( GG_STATUS_AVAIL );
			break;
			}

		case SET_BRB:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_BRB )\n" );
			if( iNetwork->GetSession() )
				{
				gg_change_status( iNetwork->GetSession(), GG_STATUS_BUSY );
				iNetwork->SetStatus( GG_STATUS_BUSY );
				BMessenger( this ).SendMessage( BEGG_UPDATE_LIST );
				}
			else
				iNetwork->Login( GG_STATUS_BUSY );
			break;
			}
		
		case SET_INVIS:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_INVIS )\n" );
			if( iNetwork->GetSession() )
				{
				gg_change_status( iNetwork->GetSession(), GG_STATUS_INVISIBLE );
				iNetwork->SetStatus( GG_STATUS_INVISIBLE );
				BMessenger( this ).SendMessage( BEGG_UPDATE_LIST );
			}
			else
				iNetwork->Login( GG_STATUS_INVISIBLE );
			break;
			}

		case SET_NOT_AVAIL:
			{
			fprintf( stderr, "MainWindow::MessageReceived( SET_NOT_AVAIL )\n" );
			if( iNetwork->GetSession() )
				{
				iNetwork->Logout();
				}
			break;
			}
		
		case BEGG_UPDATE_STATUS:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_UPDATE_STATUS )\n" );
			switch( iNetwork->GetStatus() )
				{
				case GG_STATUS_NOT_AVAIL:
					{
					iNotAvail->SetMarked( true );
					break;
					}

				case GG_STATUS_INVISIBLE:
					{
					iInvis->SetMarked( true );
					break;
					}
					
				case GG_STATUS_BUSY:
					{
					iBrb->SetMarked( true );
					break;
					}
					
				case GG_STATUS_AVAIL:
					{
					iAvail->SetMarked( true );
					break;
					}
					
				case GG_STATUS_AVAIL_DESCR:
				case GG_STATUS_BUSY_DESCR:
				case GG_STATUS_INVISIBLE_DESCR:
				case GG_STATUS_NOT_AVAIL_DESCR:
					{
					iDescr->SetMarked( true );
					break;			
					}
				}
			break;
			}

		case BEGG_UPDATE_LIST:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_UPDATE_LIST )\n" );
			List *listAvail = new List( 512 );
			List *listNotAvail = new List( 512 );
			
			GaduListItem *g = NULL;
			Person *p = NULL;
			for( int i = 0; i < iProfile->GetUserlist()->GetList()->CountItems(); i++ )
				{
				p = ( Person* ) iProfile->GetUserlist()->GetList()->ItemAt( i );
				
				if( p->GetUIN() == iProfile->GetUIN() )
					{
					g = new GaduListItem( p->GetDisplay(),
										  iNetwork->GetStatus(),
										  iProfile->GetDescription(),
										  &iResources );
					if( iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL ||
						iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL_DESCR ||
						iNetwork->GetStatus() == GG_STATUS_INVISIBLE ||
						iNetwork->GetStatus() == GG_STATUS_INVISIBLE_DESCR )
						{
						listNotAvail->AddItem( g );
						}
					else
						{
						listAvail->AddItem( g );
						}
					}
				else
					{
					if( iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL ||
						iNetwork->GetStatus() == GG_STATUS_NOT_AVAIL_DESCR )
						{
						BString *empty = new BString();
						empty->SetTo( "" );
						g = new GaduListItem( p->GetDisplay(),
											  p->GetStatus(),
											  empty,
											  &iResources );
						}
					else
						g = new GaduListItem( p->GetDisplay(),
											  p->GetStatus(),
											  p->GetDescription(),
											  &iResources );
					
					if( p->GetStatus() == GG_STATUS_NOT_AVAIL ||
						p->GetStatus() == GG_STATUS_NOT_AVAIL_DESCR )
						{
						listNotAvail->AddItem( g );
						}
					else
						{
						listAvail->AddItem( g );
						}
					}
				}
			if( iListView->CountItems() > 0 )
				{
				GaduListItem *a = NULL;
				for( int i = 0; i < iListaView->CountItems(); i++ )
					{
					a = ( GaduListItem* ) iListaView->ItemAt( i );
					delete a;
					}
				iListView->MakeEmpty();
				}
			listAvail->SortItems( SortUsers );
			iListView->AddList( listAvail );
			listNotAvail->SortItems( SortUsers );
			iListView->AddList( listNotAvail );
			iListView->Invalidate();
			delete listAvail;
			delete listNotAvail;
			break;
			}

		case BEGG_IMPORT_LIST:
			{
			fprintf( stderr, "MainWindow::MessageReceived( BEGG_IMPORT_LIST )\n" );
			if( iNetwork->GetSession() )
				iProfile->GetUserlist()->Import( iNetwork->GetSession(),
												 iProfile->GetUserlist()->GetList() );
			else
				{
				BAlert *alert = new BAlert( "Lista",
											"Musisz być połączony by zaimportować listę",
											"Ok" );
				alert->Go();
				}
			break;
			}

		default:
			BWindow::MessageReceived( aMessage );
			break;
		}
	}

int MainWindow::SortUsers(const void *aLeft, const void *aRight)
	{
	const GaduListItem 	**firstItem ( ( const GaduListItem ** ) aLeft ),
						**secondItem( ( const GaduListItem ** ) aRight );
	BString users[ 2 ];
	
	users[ 0 ] = ( *firstItem  )->GetName().String();
	users[ 1 ] = ( *secondItem )->GetName().String();
	return users[ 0 ].ICompare( users[ 1 ] );
	}

void MainWindow::LoadIcons()
	{
	fprintf( stderr, "MainWindow::LoadIcons()\n" );
	iIconAvail 	= GetBitmap( "Avail.png" );
	iIconBrb = GetBitmap( "Busy.png" );
	iIconInvis = GetBitmap( "Invisible.png" );
	iIconNotAvail = GetBitmap( "NotAvail.png" );
	iIconAvailDescr = GetBitmap( "AvailDescr.png" );
	iIconBrbDescr = GetBitmap( "BusyDescr.png" );
	iIconInvisDescr = GetBitmap( "InvisibleDescr.png" );
	iIconNotAvailDescr = GetBitmap( "NotAvailDescr.png" );
	}

void MainWindow::SetProfile( BString *aProfile )
	{
	fprintf( stderr, "MainWindow::SetProfile()\n" );
	iProfile->Load( aProfile );
	SetTitle( iProfile->GetProfileName()->String() );
	if( iProfile->GetUserlist()->IsValid() )
		{
		iListItems = iProfile->GetUserlist()->GetList();
		BMessenger( this ).SendMessage( BEGG_UPDATE_LIST );
		}
	/*
	if( fProfil->fAutoStatus != GG_STATUS_NOT_AVAIL )
	{
		if(fSiec->fSesja)
			gg_change_status(fSiec->fSesja, fProfil->fAutoStatus);
		else
			fSiec->Login(fProfil->fAutoStatus);
	}
	*/
	}

BBitmap *MainWindow::GetBitmap( const char *aName )
	{
	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
	status_t 	error;	

	const void *data = fResources.LoadResource( 'BBMP', aName, &len );

	BMemoryIO stream( data, len );
	
	BMessage archive;
	error = archive.Unflatten( &stream );
	if( error != B_OK )
		return NULL;
	bitmap = new BBitmap( &archive );
	if( !bitmap )
		return NULL;

	if( bitmap->InitCheck() != B_OK )
		{
		delete bitmap;
		return NULL;
		}
	
	return bitmap;
	}
