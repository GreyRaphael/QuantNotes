# FIX protocol

- [FIX protocol](#fix-protocol)
  - [FIX client by cpp](#fix-client-by-cpp)


## FIX client by cpp

`vcpkg install quickfix`

```bash
├── client.cfg
├── CMakeLists.txt
├── main.cpp
└── TradeClient.h
```

```ini
# client.cfg
[DEFAULT]
# Basic connection settings
ConnectionType=initiator
UseDataDictionary=N
ResetOnLogon=Y
ReconnectInterval=60
FileStorePath=store
FileLogPath=log
HeartBtInt=30

# Session active time settings
StartDay=Sunday
EndDay=Saturday
StartTime=00:00:00
EndTime=23:59:59

# Logon and logout settings
LogonDay=Sunday
LogoutDay=Saturday
LogonTime=00:00:00
LogoutTime=23:59:59

[SESSION]
# Session identification
BeginString=FIX.4.2
SenderCompID=TEST3
TargetCompID=I2test

# Network settings
SocketConnectHost=127.0.0.1
SocketConnectPort=9008
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

find_package(quickfix CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE quickfix)
```

```cpp
// main.cpp
#include <quickfix/FileLog.h>
#include <quickfix/SocketInitiator.h>

#include "TradeClient.h"

int main(int argc, char** argv) {
    FIX::SessionSettings settings("client.cfg");
    TradeClient application;
    FIX::FileStoreFactory storeFactory(settings);
    FIX::FileLogFactory logFactory(settings);
    FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

    initiator.start();

    std::cout << "Press <ENTER> to quit" << std::endl;
    std::cin.get();
    application.sendOrder();
    std::cin.get();

    initiator.stop();
}
```

```cpp
// TradeClient.h
#include <quickfix/FieldTypes.h>
#include <quickfix/FixFields.h>
#include <quickfix/FixValues.h>

#include "quickfix/Application.h"
#include "quickfix/FileStore.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Session.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/NewOrderSingle.h"

class TradeClient : public FIX::Application, public FIX::MessageCracker {
   public:
    TradeClient() {}

    void onCreate(const FIX::SessionID&) override {}
    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon - " << sessionID << std::endl;
    }
    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout - " << sessionID << std::endl;
    }
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID)
        QUICKFIX_THROW(FIX::DoNotSend) override {
        std::cout << "Sending message: " << message << std::endl;
    }
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
        std::cout << "Received message: " << message << std::endl;
        crack(message, sessionID);
    }

    void onMessage(const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received Execution Report: " << message << std::endl;
    }

    void sendOrder() {
        FIX::SessionID sessionID("FIX.4.2", "TEST3", "I2test");
        FIX42::NewOrderSingle newOrderSingle(
            FIX::ClOrdID("111111111"),
            FIX::HandlInst(FIX::HandlInst_AUTOMATED_EXECUTION_ORDER_PRIVATE_NO_BROKER_INTERVENTION),
            FIX::Symbol("AAPL"),
            FIX::Side(FIX::Side_BUY),
            FIX::TransactTime(),
            FIX::OrdType(FIX::OrdType_LIMIT));

        newOrderSingle.set(FIX::OrderQty(100));
        newOrderSingle.set(FIX::Price(100.2));
        newOrderSingle.set(FIX::Currency("CNY"));
        newOrderSingle.set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
        newOrderSingle.set(FIX::Text("gewei order"));
        FIX::Session::sendToTarget(newOrderSingle, sessionID);
    }
};
```