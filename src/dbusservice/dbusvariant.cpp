// Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusvariant.h"
#include <QDebug>

Inhibit::Inhibit()
{
    // qInfo() << "Inhibit constructor called";
}
Inhibit::~Inhibit()
{
    // qInfo() << "Inhibit destructor called";
}

void Inhibit::registerMetaType()
{
    // qInfo() << "Registering Inhibit meta type";
    qRegisterMetaType<Inhibit>("Inhibit");
    qDBusRegisterMetaType<Inhibit>();
    qRegisterMetaType<InhibitorsList>("InhibitorsList");
    qDBusRegisterMetaType<InhibitorsList>();
    // qInfo() << "Inhibit meta type registration finished";
}

QDBusArgument &operator<<(QDBusArgument &argument, const Inhibit &obj)
{
    // qInfo() << "Serializing Inhibit object";
    argument.beginStructure();
    argument << obj.what << obj.who << obj.why << obj.mode << obj.uid << obj.pid;
    argument.endStructure();
    // qInfo() << "Inhibit object serialization finished";
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Inhibit &obj)
{
    // qInfo() << "Deserializing Inhibit object";
    argument.beginStructure();
    argument >> obj.what >> obj.who >> obj.why >> obj.mode >> obj.uid >> obj.pid;
    argument.endStructure();
    // qInfo() << "Inhibit object deserialization finished";
    return argument;
}

UserInfo::UserInfo()
{
    // qInfo() << "UserInfo constructor called";
}
UserInfo::~UserInfo()
{
    // qInfo() << "UserInfo destructor called";
}

void UserInfo::registerMetaType()
{
    // qInfo() << "Registering UserInfo meta type";
    qRegisterMetaType<UserInfo>("UserInfo");
    qDBusRegisterMetaType<UserInfo>();
    qRegisterMetaType<UserList>("UserList");
    qDBusRegisterMetaType<UserList>();
    // qInfo() << "UserInfo meta type registration finished";
}

QDBusArgument &operator<<(QDBusArgument &argument, const UserInfo &obj)
{
    // qInfo() << "Serializing UserInfo object";
    argument.beginStructure();
    argument << obj.pid << obj.id << obj.userName;
    argument.endStructure();
    // qInfo() << "UserInfo object serialization finished";
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UserInfo &obj)
{
    // qInfo() << "Deserializing UserInfo object";
    argument.beginStructure();
    argument >> obj.pid >> obj.id >> obj.userName;
    argument.endStructure();
    // qInfo() << "UserInfo object deserialization finished";
    return argument;
}

SeatInfo::SeatInfo()
{
    // qInfo() << "SeatInfo constructor called";
}
SeatInfo::~SeatInfo()
{
    // qInfo() << "SeatInfo destructor called";
}

void SeatInfo::registerMetaType()
{
    // qInfo() << "Registering SeatInfo meta type";
    qRegisterMetaType<SeatInfo>("SeatInfo");
    qDBusRegisterMetaType<SeatInfo>();
    qRegisterMetaType<SeatList>("SeatList");
    qDBusRegisterMetaType<SeatList>();
    // qInfo() << "SeatInfo meta type registration finished";
}

QDBusArgument &operator<<(QDBusArgument &argument, const SeatInfo &obj)
{
    // qInfo() << "Serializing SeatInfo object";
    argument.beginStructure();
    argument << obj.id << obj.seat;
    argument.endStructure();
    // qInfo() << "SeatInfo object serialization finished";
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SeatInfo &obj)
{
    // qInfo() << "Deserializing SeatInfo object";
    argument.beginStructure();
    argument >> obj.id >> obj.seat;
    argument.endStructure();
    // qInfo() << "SeatInfo object deserialization finished";
    return argument;
}

SessionInfo::SessionInfo()
{
    // qInfo() << "SessionInfo constructor called";
}
SessionInfo::~SessionInfo()
{
    // qInfo() << "SessionInfo destructor called";
}

void SessionInfo::registerMetaType()
{
    // qInfo() << "Registering SessionInfo meta type";
    qRegisterMetaType<SessionInfo>("SessionInfo");
    qDBusRegisterMetaType<SessionInfo>();
    qRegisterMetaType<SessionList>("SessionList");
    qDBusRegisterMetaType<SessionList>();
    // qInfo() << "SessionInfo meta type registration finished";
}

QDBusArgument &operator<<(QDBusArgument &argument, const SessionInfo &obj)
{
    // qInfo() << "Serializing SessionInfo object";
    argument.beginStructure();
    argument << obj.session << obj.pid << obj.user << obj.id << obj.seat;
    argument.endStructure();
    // qInfo() << "SessionInfo object serialization finished";
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SessionInfo &obj)
{
    // qInfo() << "Deserializing SessionInfo object";
    argument.beginStructure();
    argument >> obj.session >> obj.pid >> obj.user >> obj.id >> obj.seat;
    argument.endStructure();
    // qInfo() << "SessionInfo object deserialization finished";
    return argument;
}
