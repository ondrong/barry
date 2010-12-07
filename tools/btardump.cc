///
/// \file	tardump.cc
///		Utility to dump tarball	backup records to stdout.
///

/*
    Copyright (C) 2010, Net Direct Inc. (http://www.netdirect.ca/)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the GNU General Public License in the COPYING file at the
    root directory of this project for more details.
*/

#include <barry/barry.h>
#ifdef __BARRY_SYNC_MODE__
#include <barry/barrysync.h>
#endif
#include <barry/barrybackup.h>
#include <iostream>
#include <iomanip>
#include <getopt.h>

using namespace std;
using namespace Barry;

#ifdef __BARRY_SYNC_MODE__
template <class Record>
class MimeDump
{
public:
	void Dump(std::ostream &os, const Record &rec)
	{
		os << rec << endl;
	}

	static bool Supported() { return false; }
};

template <>
class MimeDump<Contact>
{
public:
	void Dump(std::ostream &os, const Contact &rec)
	{
		Sync::vCard vcard;
		os << vcard.ToVCard(rec) << endl;
	}

	static bool Supported() { return true; }
};
#endif

class MyAllRecordDumpStore : public AllRecordDumpStore
{
	bool vformat_mode;

public:
	explicit MyAllRecordDumpStore(std::ostream &os, bool vformat_mode=false)
		: AllRecordDumpStore(os)
		, vformat_mode(vformat_mode)
	{
	}
	virtual void operator() (const Barry::Contact &);
	virtual void operator() (const Barry::Calendar &);
	virtual void operator() (const Barry::CalendarAll &);
	virtual void operator() (const Barry::Memo &);
	virtual void operator() (const Barry::Task &);
};

void MyAllRecordDumpStore::operator() (const Barry::Contact &rec)
{
	if( vformat_mode ) {
#ifdef __BARRY_SYNC_MODE__
		Sync::vCard vcard;
		m_os << vcard.ToVCard(rec) << endl;
#endif
	}
	else {
		m_os << rec << std::endl;
	}
}

void MyAllRecordDumpStore::operator() (const Barry::Calendar &rec)
{
	if( vformat_mode ) {
#ifdef __BARRY_SYNC_MODE__
		Sync::vTimeConverter vtc;
		Sync::vCalendar vcal(vtc);
		m_os << vcal.ToVCal(rec) << endl;
#endif
	}
	else {
		m_os << rec << std::endl;
	}
}

void MyAllRecordDumpStore::operator() (const Barry::CalendarAll &rec)
{
	if( vformat_mode ) {
#ifdef __BARRY_SYNC_MODE__
		Sync::vTimeConverter vtc;
		Sync::vCalendar vcal(vtc);
		m_os << vcal.ToVCal(rec) << endl;
#endif
	}
	else {
		m_os << rec << std::endl;
	}
}

void MyAllRecordDumpStore::operator() (const Barry::Memo &rec)
{
	if( vformat_mode ) {
#ifdef __BARRY_SYNC_MODE__
		Sync::vJournal vjournal;
		m_os << vjournal.ToMemo(rec) << endl;
#endif
	}
	else {
		m_os << rec << std::endl;
	}
}

void MyAllRecordDumpStore::operator() (const Barry::Task &rec)
{
	if( vformat_mode ) {
#ifdef __BARRY_SYNC_MODE__
		Sync::vTimeConverter vtc;
		Sync::vTodo vtodo(vtc);
		m_os << vtodo.ToTask(rec) << endl;
#endif
	}
	else {
		m_os << rec << std::endl;
	}
}

void Usage()
{
   int major, minor;
   const char *Version = Barry::Version(major, minor);

   cerr
   << "btardump - Command line parser for Barry backup files\n"
   << "           Copyright 2010, Net Direct Inc. (http://www.netdirect.ca/)\n"
   << "           Using: " << Version << "\n"
   << "\n"
   << "   -d db     Name of database to dump.  Can be used multiple times\n"
   << "             to parse multiple databases at once.  If not specified\n"
   << "             at all, all available databases from the backup are\n"
   << "             dumped.\n"
   << "   -h        This help\n"
#ifdef __BARRY_SYNC_MODE__
   << "   -V        Dump records using MIME vformats where possible\n"
#endif
   << "\n"
   << "   [files...] Backup file(s), created by btool or the backup GUI.\n"
   << endl;
}

int main(int argc, char *argv[])
{
	try {
		bool vformat_mode = false;

		vector<string> db_names;
		vector<string> backup_files;

		// process command line options
		for(;;) {
			int cmd = getopt(argc, argv, "d:hV");
			if( cmd == -1 )
				break;

			switch( cmd )
			{
			case 'd':	// show dbname
				db_names.push_back(string(optarg));
				break;

			case 'V':	// vformat MIME mode
#ifdef __BARRY_SYNC_MODE__
				vformat_mode = true;
#else
				cerr << "-V option not supported - no Sync "
					"library support available\n";
				return 1;
#endif
				break;

			case 'h':	// help
			default:
				Usage();
				return 0;
			}
		}

		// grab all backup filenames
		while( optind < argc ) {
			backup_files.push_back(string(argv[optind++]));
		}

		if( backup_files.size() == 0 ) {
			Usage();
			return 0;
		}



		Barry::Init();

		// create the parser, and use stdout dump objects for output
		AllRecordParser parser(cout,
			new HexDumpParser(cout),
			new MyAllRecordDumpStore(cout, vformat_mode));

		for( size_t i = 0; i < backup_files.size(); i++ ) {

			cout << "Reading file: " << backup_files[i] << endl;

			Restore builder(backup_files[i]);

			// add desired database names
			for( size_t j = 0; j < db_names.size(); j++ ) {
				builder.AddDB(db_names[i]);
			}

			// create the pipe to connect builder to parser and
			// move the data
			Pipe pipe(builder);
			pipe.PumpFile(parser);
		}

	}
	catch( exception &e ) {
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}

