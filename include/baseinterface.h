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

#ifndef BASEINTERFACE_H
#define BASEINTERFACE_H

#include "abstractinterface.h"

class BaseInterface : public AbstractInterface {
public:
    BaseInterface (int id, QString name)
    : id_ (id)
    , name_ (name)
    {
    }

    ~BaseInterface () {}

    int getId () const { return id_; }
    const QString& getName () const { return name_; }
    QString getTypeName () const { return type_; }
    BaseUI *getUI () const { return ui_; }

protected:
    void setName (QString newName) { name_ = newName; }
    void setTypeName (QString newType) { type_ = newType; }
    void setUI (BaseUI *ui) { ui_ = ui; }
private:
    int id_;
    QString name_;
    QString type_;
    BaseUI *ui_;
};

#endif // BASEINTERFACE_H
