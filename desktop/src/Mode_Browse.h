///
/// \file	Mode_Browse.h
///		Mode derived class for database browsing
///

/*
    Copyright (C) 2011-2013, Net Direct Inc. (http://www.netdirect.ca/)

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

#ifndef __BARRYDESKTOP_MODE_BROWSE_H__
#define __BARRYDESKTOP_MODE_BROWSE_H__

#include "Mode.h"
#include "util.h"
#include <barry/barry.h>
#include <barry/barrysync.h>
#include <barry/scoped_lock.h>
#include <tr1/memory>
#include <list>
#include "wxi18n.h"

class GUIDesktopConnector : public Barry::DesktopConnector
{
	wxWindow *m_parent;

public:
	GUIDesktopConnector(wxWindow *parent, const char *password,
		const std::string &locale, const Barry::ProbeResult &result)
		: Barry::DesktopConnector(password, locale, result)
		, m_parent(parent)
	{
	}

	virtual bool PasswordPrompt(const Barry::BadPassword &bp,
					std::string &password_result);
};

// Holds the "right" to use the desktop, and this right expires with the
// scope of this class, and no other code can use it while this object exists.
//
// Is copied around by the below auto_ptr<> typedef.
class DesktopInstance
{
private:
	Barry::scoped_lock m_lock;

	// external interfaces... owned by caller
	GUIDesktopConnector &m_gdc;

public:
	explicit DesktopInstance(pthread_mutex_t &mutex,
				GUIDesktopConnector &gdc)
		: m_lock(mutex)
		, m_gdc(gdc)
	{
	}

	GUIDesktopConnector& Connector() { return m_gdc; }
	Barry::Mode::Desktop& Desktop() { return m_gdc.GetDesktop(); }
	Barry::IConverter& IC() { return m_gdc.GetIConverter(); }
};

typedef std::auto_ptr<DesktopInstance> DesktopInstancePtr;

//
// ThreadableDesktop
//
/// Protection class that holds the GUIDesktopConnector (i.e. the main
/// desktop instance) and a mutex.  To access the desktop object, call
/// Get(), which locks access for the caller and returns the
/// DesktopInstancePtr smart pointer.  The lock is released when
/// the DesktopInstancePtr goes out of scope.
///
class ThreadableDesktop
{
private:
	pthread_mutex_t m_mutex;

	// external interfaces... owned by caller
	GUIDesktopConnector &m_gdc;

public:
	explicit ThreadableDesktop(GUIDesktopConnector &gdc)
		: m_gdc(gdc)
	{
		if( pthread_mutex_init(&m_mutex, NULL) ) {
			throw Barry::Error(_C("Failed to create mutex for ThreadableDesktop"));
		}
	}

	DesktopInstancePtr Get()
	{
		return DesktopInstancePtr(
			new DesktopInstance(m_mutex, m_gdc));
	}
};

//
// DataCache
//
/// Abstract base class representing a single cached record in a database.
/// Does not actually contain the record data (base classes hold that), but
/// defines what can be done with such a record cache object. (eg. such as
/// editing the record)
///
class DataCache
{
public:
	typedef Barry::RecordStateTable::IndexType IndexType;

private:
	IndexType m_state_index;
	uint32_t m_record_id;

public:
	DataCache(IndexType state_index, uint32_t record_id)
		: m_state_index(state_index)
		, m_record_id(record_id)
	{
	}

	virtual ~DataCache() {}

	virtual IndexType GetStateIndex() const { return m_state_index; }
	virtual uint32_t GetRecordId() const { return m_record_id; }
	virtual void SetIds(IndexType state_index, uint32_t record_id)
	{
		std::cout << "DataCache::SetIds(" << state_index << ", " << record_id << ");" << std::endl;
		m_state_index = state_index;
		m_record_id = record_id;
	}

	// set editable to false if you just want to view record
	// non-buildable records will always be non-editable regardless
	virtual bool Edit(wxWindow *parent, bool editable,
		const Barry::TimeZones &zones) = 0;
	virtual bool Card(wxWindow *parent, std::string &vdata) {return false;}
	virtual std::string GetDescription() const = 0;

	virtual bool IsBuildable() const { return false; }

	bool operator< (const DataCache &other)
	{
		return GetDescription() < other.GetDescription();
	}

};

//
// DataCachePtr
//
/// Basically a shared_ptr<DataCache>, but with the added operator<()
/// overload so that it calls DataCache's operator<(), for sorting
/// of objects via this pointer.
///
class DataCachePtr : public std::tr1::shared_ptr<DataCache>
{
public:
	DataCachePtr()
	{
	}

	explicit DataCachePtr(DataCache *obj)
		: std::tr1::shared_ptr<DataCache>(obj)
	{
	}

	bool operator< (const DataCachePtr &other)
	{
		return (*this)->operator<( *other );
	}
};

//
// DBDataCache
//
/// Derived class from DataCache that holds the raw data of a record that
/// does not yet have a parser.  Unable to do much with such records in
/// terms of editing, but still able to be cached in the DBMap.
///
class DBDataCache : public DataCache
{
	Barry::DBData m_raw;

public:
	DBDataCache(DataCache::IndexType index, const Barry::DBData &raw);

	virtual bool Edit(wxWindow *parent, bool editable,
		const Barry::TimeZones &zones);
	virtual std::string GetDescription() const;
};

// returns true if GUI indicates change (i.e. if dialogreturns wxID_OK)
#undef HANDLE_PARSER
#define HANDLE_PARSER(dbname) \
	bool EditRecord(wxWindow *parent, bool editable, \
		const Barry::TimeZones &zones, Barry::dbname &rec);
ALL_KNOWN_PARSER_TYPES

//
// RecordCache<>
//
/// Template class derived from DataCache which holds the parsed record data
/// for the templated record type.  Allows actions such as editing.
///
template <class RecordT>
class RecordCache
	: public DataCache
	, public Barry::Builder
{
private:
	RecordT m_rec;

public:
	RecordCache(DataCache::IndexType index, const RecordT &rec)
		: DataCache(index, rec.GetUniqueId())
		, m_rec(rec)
	{
		// if copying from another record, don't copy what
		// we don't understand
		m_rec.Unknowns.clear();
	}

	const RecordT& GetRecord() const { return m_rec; }

	// hook SetIds() to grab any new record_ids / UniqueIDs
	virtual void SetIds(IndexType state_index, uint32_t record_id)
	{
		std::cout << "RecordCache::SetIds(" << state_index << ", " << record_id << ");" << std::endl;
		DataCache::SetIds(state_index, record_id);
		m_rec.SetIds(RecordT::GetDefaultRecType(), record_id);
	}

	virtual bool Edit(wxWindow *parent, bool editable,
		const Barry::TimeZones &zones)
	{
		RecordT copy = m_rec;
		bool changed = EditRecord(parent, editable, zones, copy);
		// FIXME - we could, at this point, add a (copy != m_rec)
		// check here, to prevent writing a record that has not
		// changed, but that would require using the FieldHandle<>
		// system (a lot of code), and it's not a critical feature
		// right now
		if( changed && editable ) {
			m_rec = copy;
			return true;
		}
		return false;
	}

	virtual bool Card(wxWindow *parent, std::string &vdata)
	{
		if( !Barry::MimeDump<RecordT>::Supported() )
			return false;

		std::ostringstream oss;
		Barry::MimeDump<RecordT>::Dump(oss, m_rec);
		vdata = oss.str();
		return true;
	}

	virtual std::string GetDescription() const
	{
		return m_rec.GetDescription();
	}

	virtual bool IsBuildable() const
	{
		return ::IsBuildable<RecordT>();
	}

	//
	// Barry::Builder overrides
	//
	virtual bool BuildRecord(Barry::DBData &data, size_t &offset,
		const Barry::IConverter *ic)
	{
		Barry::SetDBData<RecordT>(m_rec, data, offset, ic);
		return true;
	}

	virtual bool FetchRecord(Barry::DBData &data,
		const Barry::IConverter *ic)
	{
		size_t offset = 0;
		Barry::SetDBData<RecordT>(m_rec, data, offset, ic);
		return true;
	}

	virtual bool EndOfFile() const
	{
		return true;
	}
};

//
// DBCache
//
/// This is a container class that holds a std::list of DataCache-derived
/// objects for all the records of a single database.
///
class DBCache : public Barry::AllRecordStore, public Barry::Parser
{
public:
	typedef Barry::RecordStateTable::IndexType	IndexType;
	typedef std::list<DataCachePtr>			DataList;
	typedef DataList::iterator			iterator;
	typedef DataList::const_iterator		const_iterator;

private:
	ThreadableDesktop &m_tdesktop;
	std::string m_dbname;
	unsigned int m_dbid;
	Barry::RecordStateTable m_state;
	DataList m_records;

	// per-record load state
	IndexType m_index;

protected:
	// helper functions, to work around Tasks bug in device
	bool OverwriteRecord(wxWindow *parent, iterator record);
	bool DeleteAndAddRecord(wxWindow *parent, iterator record);

public:
	// loads records in constructor
	DBCache(ThreadableDesktop &tdesktop, const std::string &dbname);
	virtual ~DBCache();

	const std::string& GetDBName() { return m_dbname; }

	iterator Get(int list_offset);
	const_iterator Get(int list_offset) const;
	// returns the numeric index of the record, to keep with GUI
	int GetIndex(iterator record) const;
	int GetIndex(const_iterator record) const;

	iterator Add(wxWindow *parent, DataCachePtr p);	// adds to device too,
							// just like Add() below
	iterator Add(wxWindow *parent, const Barry::TimeZones &zones,
		iterator copy_record);
	bool Edit(wxWindow *parent, const Barry::TimeZones &zones,
		iterator record);
	bool Delete(wxWindow *parent, iterator record);

	iterator begin() { return m_records.begin(); }
	iterator end() { return m_records.end(); }
	const_iterator begin() const { return m_records.begin(); }
	const_iterator end() const { return m_records.end(); }

	// For Barry::AllRecordStore
#undef HANDLE_PARSER
#define HANDLE_PARSER(tname) \
        virtual void operator() (const Barry::tname &);
	ALL_KNOWN_PARSER_TYPES

	// For Barry::Parser
	virtual void ParseRecord(const Barry::DBData &data,
		const Barry::IConverter *ic);
};

//
// DBMap
//
/// A std::map based container that maps database names with their
/// corresponding DBCaches.
///
class DBMap
{
public:
	typedef std::tr1::shared_ptr<DBCache>		DBCachePtr;
	typedef std::map<std::string, DBCachePtr>	MapType;

private:
	ThreadableDesktop &m_tdesktop;
	MapType m_map;
	pthread_mutex_t m_map_mutex;
	pthread_mutex_t m_load_mutex;

public:
	DBMap(ThreadableDesktop &tdesktop);

	DBCachePtr LoadDBCache(const std::string &dbname);
	DBCachePtr GetDBCache(const std::string &dbname);
};

//
// BrowseMode
//
/// The Desktop GUI mode class for browsing databases and manipulating records.
///
class BrowseMode : public wxEvtHandler, public Mode
{
private:
	DECLARE_EVENT_TABLE()	// sets to protected:

private:
	wxWindow *m_parent;

	// device interface
	std::auto_ptr<GUIDesktopConnector> m_con;
	std::auto_ptr<ThreadableDesktop> m_tdesktop;
	std::auto_ptr<DBMap> m_dbmap;
	Barry::DatabaseDatabase m_dbdb;
	std::auto_ptr<Barry::TimeZones> m_zones;

	// window controls
	std::auto_ptr<wxBoxSizer> m_top_sizer;
	std::auto_ptr<wxListCtrl> m_dbdb_list;
	std::auto_ptr<wxListCtrl> m_record_list;
	std::auto_ptr<wxCheckBox> m_show_all_checkbox;
	std::auto_ptr<wxButton> m_import_record_button;
	std::auto_ptr<wxButton> m_export_record_button;
	std::auto_ptr<wxButton> m_add_record_button;
	std::auto_ptr<wxButton> m_copy_record_button;
	std::auto_ptr<wxButton> m_edit_record_button;
	std::auto_ptr<wxButton> m_delete_record_button;
	std::auto_ptr<wxStaticText> m_load_status_text;

	// misc supporting data
	wxString m_device_id_str;

	// GUI state
	bool m_buildable;		// true if currently displayed db has
					// a Builder available for it
	bool m_editable;		// true if currently displayed db has
					// an edit dialog available for it
	bool m_cardable;		// true if currently displayed db is
					// capable of MIME style import/exports
	bool m_show_all;		// if true, show all databases in list
					// instead of just the parsable ones
	std::string m_current_dbname;
	long m_current_record_item;

	// thread state
	pthread_t m_cache_thread;
	volatile bool m_abort_flag;

protected:
	void CreateControls();
	int GUItoDBDBIndex(int gui_index);
	void FillDBDBList();
	void FillRecordList(const std::string &dbname);
	void UpdateButtons();
	void FillCache();

	// background thread function
	static void* FillCacheThread(void *bobj);

public:
	BrowseMode(wxWindow *parent, const Barry::ProbeResult &device);
	~BrowseMode();

	void SendStatusEvent(const std::string &dbname);

	// virtual override events (derived from Mode)
	wxString GetTitleText() const { return _W("Barry Database Browser"); }

	// window events
	void OnDBDBListSelChange(wxListEvent &event);
	void OnRecordListSelChange(wxListEvent &event);
	void OnRecordListActivated(wxListEvent &event);
	void OnShowAll(wxCommandEvent &event);
	void OnImportRecord(wxCommandEvent &event);
	void OnExportRecord(wxCommandEvent &event);
	void OnAddRecord(wxCommandEvent &event);
	void OnCopyRecord(wxCommandEvent &event);
	void OnEditRecord(wxCommandEvent &event);
	void OnDeleteRecord(wxCommandEvent &event);
	void OnStatusEvent(wxCommandEvent &event);
};

#endif

