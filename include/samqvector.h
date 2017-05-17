/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SAMQVECTOR_H
#define SAMQVECTOR_H

#include <QVector>

namespace Sam {
    template<typename T>
    struct vector_traits< QVector<T> > {
        typedef T value_type;
        typedef QVector< QVector<T> > vecvec_type;

        static void do_reserve (QVector<T> & v, unsigned int n) {
            v.reserve (n);
        }

        template<typename V>
        static void do_fill (QVector<T> & v, unsigned int length, V val) {
            v.fill(val, length);
        }
    };
};

#endif // SAMQVECTOR_H
