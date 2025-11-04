/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqn2csv/model.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * $Id: model.h 17557 2025-10-31 13:23:31Z greg $
 *
 * ------------------------------------------------------------------------
 */

#pragma once
#include <iostream>
#include "model.h"

namespace Output {
    std::ostream& rows( std::ostream& output, const std::vector<std::vector<Model::Value>>& data );
    std::ostream& columns( std::ostream& output, const std::vector<std::vector<Model::Value>>& data, size_t max_columns );
};

