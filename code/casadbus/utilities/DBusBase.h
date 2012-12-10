#ifndef CASA_DBUS_BASE_H__
#define CASA_DBUS_BASE_H__

#include <boost/thread/thread.hpp>
#include <dbus-c++/dbus.h>

namespace casa {

  class Record;

class DBusThreadedBase
{
 public:
  DBusThreadedBase();
  ~DBusThreadedBase();
  
  void stopService();

  /* Methods to make translating to/from the map easy */
  static std::map<std::string,DBus::Variant> fromRecord(Record);
  static Record toRecord(std::map<std::string,DBus::Variant>);

 private:
  
  void serviceLoop();
  boost::thread  *itsThread;
};

  class DBusService : public DBus::IntrospectableAdaptor,
                      public DBus::ObjectAdaptor,
                      public DBusThreadedBase
{
 public:
  DBusService(const std::string &serviceName);
  ~DBusService();
};

  class ServiceProxy : public DBus::IntrospectableProxy,
                       public DBus::ObjectProxy,
                       public DBusThreadedBase
{
 public:
  ServiceProxy(const std::string& serviceName);
  ~ServiceProxy();
};

}
    
#endif
