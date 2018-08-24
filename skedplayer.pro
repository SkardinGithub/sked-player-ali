TEMPLATE = subdirs

SUBDIRS = lib
example {
  SUBDIRS += example
  example.depends += lib
}
server {
  SUBDIRS += server
  server.depends += lib
}
