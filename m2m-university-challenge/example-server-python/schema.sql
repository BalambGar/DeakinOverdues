drop table if exists records;
drop table if exists rfids;
drop table if exists users;
drop table if exists workouts;
create table records (
  id integer primary key autoincrement,
  imei text not null,
  imsi text not null,
  time_stamp text not null,
  cpu_ID text not null,
  display_string text not null,
  lat text not null,
  lon text not null
);

create table rfids (
 userID integer,
 rfid integer primary key
);

create table users (
 id integer primary key autoincrement,
 fname text not null,
 lname text not null
);

create table workouts (
 id integer primary key autoincrement,
 userID integer not null,
 averageDistance real not null,
 totalReps integer not null 
);

insert into rfids values (1, 172370816);
insert into rfids values (2, -1303356269);
insert into users values (1, "Test", "Name");
insert into users values (2, "John", "Smith");
