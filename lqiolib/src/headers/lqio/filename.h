/* -*- c++ -*-
 * $Id: filename.h 17358 2024-10-11 11:15:09Z greg $
 *
 * MVA solvers: Exact, Bard-Schweitzer, Linearizer and Linearizer2.
 * Abstract superclass does no operation by itself.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */

#ifndef LQIOLIB_FILENAME_H
#define	LQIOLIB_FILENAME_H

#include <string>
#include <filesystem>

namespace LQIO {

    class Filename
    {
    public:
	Filename() {};
	Filename( const std::filesystem::path& base, const std::string& extension = "", const std::filesystem::path& directory = "", const std::string& suffix = "");
	Filename( const Filename& );
	Filename& operator=( const Filename& );
	Filename& operator=( const std::string& );
	Filename& operator=( const std::filesystem::path& );

	const std::filesystem::path& operator()() const { return _path; }
	std::string str() const { return _path.string(); }
	Filename& operator<<( const std::string& );
	Filename& operator<<( const unsigned );

	const std::filesystem::path& generate( const std::filesystem::path& directory, const std::filesystem::path& path, const std::string& suffix, const std::string& extension );
	const std::filesystem::path& generate( const std::filesystem::path& path, const std::string& extension ) { return generate( std::filesystem::path(), path, extension, std::string() ); } // 

	Filename& backup() { Filename::backup( (*this)() ); return *this; }

	int mtimeCmp( const std::string& file_name );

	static void backup( const std::filesystem::path& file_name );
	static std::filesystem::path createDirectory( const std::filesystem::path& name, bool lqx_output );
        static inline bool isFileName( const std::filesystem::path& name ) { return !name.empty() && name != "-"; }

    private:
	std::filesystem::path _path;
    };
}
#endif
