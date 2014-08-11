#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "LeaderStatus.h"

//------------------------------------------------------------------------------
struct Message {
  uint32_t sequence_number;
  uint32_t ack_sequence_number;

  virtual std::string label() const = 0;
  virtual void to_stream(std::ostream&) const = 0;

  Message(uint32_t sequence_number, uint32_t ack_sequence_number)
    : sequence_number(sequence_number)
    , ack_sequence_number(ack_sequence_number)
  {}

  Message(std::istream& is) { is >> sequence_number >> ack_sequence_number; }
  
  virtual ~Message() {}
};

inline std::ostream& operator<<(std::ostream& os, const Message& msg) {
  return os << msg.sequence_number << " " << msg.ack_sequence_number;
}

//------------------------------------------------------------------------------
template<class T> struct MessageTemplate : Message {
  MessageTemplate(uint32_t sequence_number, uint32_t ack_sequence_number)
    : Message(sequence_number, ack_sequence_number)
  {}

  MessageTemplate(std::istream& is) : Message(is) {}

  void to_stream(std::ostream& os) const override {
    os << static_cast<const T&>(*this);
  }
};

//------------------------------------------------------------------------------
struct PingMsg : MessageTemplate<PingMsg> {
  std::string label() const override { return "ping"; }

  PingMsg(uint32_t sequence_number, uint32_t ack_sequence_number)
    : MessageTemplate(sequence_number, ack_sequence_number) {}

  PingMsg(std::istream& is) : MessageTemplate(is) {}
};

inline std::ostream& operator<<(std::ostream& os, const PingMsg& ping) {
  return os << static_cast<const Message&>(ping);
}

//------------------------------------------------------------------------------
struct StartMsg : MessageTemplate<StartMsg> {
  std::string label() const override { return "start"; }

  StartMsg(uint32_t sequence_number, uint32_t ack_sequence_number)
    : MessageTemplate(sequence_number, ack_sequence_number) {}

  StartMsg(std::istream& is) : MessageTemplate(is) {}
};

inline std::ostream& operator<<(std::ostream& os, const StartMsg& msg) {
  return os << static_cast<const Message&>(msg);
}

//------------------------------------------------------------------------------
struct NumberMsg : MessageTemplate<NumberMsg> {
  std::string label() const override { return "random_number"; }

  float random_number;

  NumberMsg(uint32_t sequence_number, uint32_t ack_sequence_number
       , float random_number)
    : MessageTemplate(sequence_number, ack_sequence_number)
    , random_number(random_number)
  {}

  NumberMsg(std::istream& is) : MessageTemplate(is) {
    is >> random_number;
  }
};

inline std::ostream& operator<<(std::ostream& os, const NumberMsg& msg) {
  return os << static_cast<const Message&>(msg) << " " << msg.random_number;
}

//------------------------------------------------------------------------------
struct Status1Msg : MessageTemplate<Status1Msg> {
  std::string label() const override { return "status1"; }

  LeaderStatus leader_status;

  Status1Msg(uint32_t sequence_number, uint32_t ack_sequence_number
       , LeaderStatus leader_status)
    : MessageTemplate(sequence_number, ack_sequence_number)
    , leader_status(leader_status)
  {}

  Status1Msg(std::istream& is) : MessageTemplate(is) {
    is >> leader_status;
  }
};

inline std::ostream& operator<<( std::ostream& os
                               , const Status1Msg& msg) {
  return os << static_cast<const Message&>(msg) << " " << msg.leader_status;
}

//------------------------------------------------------------------------------
struct Status2Msg : MessageTemplate<Status2Msg> {
  std::string label() const override { return "status2"; }

  LeaderStatus leader_status;

  Status2Msg(uint32_t sequence_number, uint32_t ack_sequence_number
       , LeaderStatus leader_status)
    : MessageTemplate(sequence_number, ack_sequence_number)
    , leader_status(leader_status)
  {}

  Status2Msg(std::istream& is) : MessageTemplate(is) {
    is >> leader_status;
  }
};

inline std::ostream& operator<<( std::ostream& os
                               , const Status2Msg& msg) {
  return os << static_cast<const Message&>(msg) << " " << msg.leader_status;
}

//------------------------------------------------------------------------------
template< typename PingHandler
        , typename StartHandler
        , typename NumberHandler
        , typename Status1Handler
        , typename Status2Handler
        >
void dispatch_message( std::istream& is
                     , const PingHandler&    ping_handler
                     , const StartHandler&   start_handler
                     , const NumberHandler&  random_number_handler
                     , const Status1Handler& status1_handler
                     , const Status2Handler& status2_handler) {
  using namespace std;

  string label;
  is >> label;

  if (label == "ping") {
    ping_handler(PingMsg(is));
  }
  else if (label == "start") {
    start_handler(StartMsg(is));
  }
  else if (label == "random_number") {
    random_number_handler(NumberMsg(is));
  }
  else if (label == "status1") {
    status1_handler(Status1Msg(is));
  }
  else if (label == "status2") {
    status2_handler(Status2Msg(is));
  }
  else {
    throw runtime_error("unrecognized message label");
  }
}

//------------------------------------------------------------------------------

#endif // ifndef __PROTOCOL_H__
