/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id: option.h 15155 2021-12-06 18:54:53Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQN2PS_OPTION_H
#define LQN2PS_OPTION_H

enum class Aggregate {
    NONE,
    SEQUENCES,
    ACTIVITIES,
    PHASES,
    ENTRIES,
    THREADS
};

enum class File_Format {
    EEPIC,
#if defined(EMF_OUTPUT)
    EMF,
#endif
    FIG,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    GIF,
#endif
#if JMVA_OUTPUT && HAVE_EXPAT_H
    JMVA,
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    JPEG,
#endif
    JSON,
    LQX,
    NO_OUTPUT,
    OUTPUT,
#if QNAP2_OUTPUT
    QNAP2,
#endif
    PARSEABLE,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    PNG,
#endif
    POSTSCRIPT,
    PSTEX,
    RTF,
    SRVN,
#if defined(SVG_OUTPUT)
    SVG,
#endif
#if defined(SXD_OUTPUT)
    SXD,
#endif
#if defined(TXT_OUTPUT)
    TXT,
#endif
#if defined(X11_OUTPUT)
    X11,
#endif
    XML,
    UNKNOWN
};

enum class Layering {
    BATCH,
    GROUP,
    HWSW,
    MOL,
    PROCESSOR,
    PROCESSOR_TASK,
    SHARE,
    SQUASHED,
    SRVN,
    TASK_PROCESSOR
};

enum class Special {
    ANNOTATE,
    ARROW_SCALING,
    BCMP,
    CLEAR_LABEL_BACKGROUND,
    EXHAUSTIVE_TOPOLOGICAL_SORT,
    FLATTEN_SUBMODEL,
    FORCE_INFINITE,
    FORWARDING_DEPTH,
    GROUP,
    LAYER_NUMBER,
    NONE,
    NO_ALIGNMENT_BOX,
    NO_ASYNC_TOPOLOGICAL_SORT,
    NO_CV_SQR,
    NO_PHASE_TYPE,
    NO_REF_TASK_CONVERSION,
    PROCESSOR_SCHEDULING,
    PRUNE,
    QUORUM_REPLY,
    RENAME,
    SORT,
    SPEX_HEADER,
    SQUISH_ENTRY_NAMES,
    SUBMODEL_CONTENTS,
    TASKS_ONLY,	
    TASK_SCHEDULING
};

class Options
{
private:
    Options();
    Options( const Options& );
    Options& operator=( const Options& );

public:
    static const std::map<const File_Format,const std::string> file_format;
    static const std::map<const Layering,const std::string> layering;
    static const char * colouring[];
    static const char * processor[];
    static const std::map<const Aggregate,const std::string> aggregate;
    static const char * justification[];
    static const char * integer[];
    static const char * key[];
    static const char * real[];
    static const char * replication[];
    static const char * string[];
    static const char * sort[];
    static const std::map<const Special,const std::string> special;

    static size_t find_if( const char**, const std::string& );
    static Aggregate get_aggregate( const std::string& );
    static File_Format get_file_format( const std::string& );
    static Layering get_layering( const std::string& );
    static Special get_special( const::std::string& );
};
#endif
