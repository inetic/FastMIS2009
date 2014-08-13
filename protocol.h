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
  os << msg.sequence_number << " " << msg.ack_sequence_number << " ";
  msg.to_stream(os);
  return os;
}

//------------------------------------------------------------------------------
struct PingMsg : Message {
  std::string label() const override { return "ping"; }

  PingMsg(uint32_t sequence_number, uint32_t ack_sequence_number)
    : Message(sequence_number, ack_sequence_number) {}

  PingMsg(std::istream& is) : Message(is) {}

  void to_stream(std::ostream&) const override {}
};

//------------------------------------------------------------------------------
struct StartMsg : Message {
  std::string label() const override { return "start"; }

  StartMsg(uint32_t sequence_number, uint32_t ack_sequence_number)
    : Message(sequence_number, ack_sequence_number) {}

  StartMsg(std::istream& is) : Message(is) {}

  void to_stream(std::ostream&) const override {}
};

//------------------------------------------------------------------------------
struct NumberMsg : Message {
  std::string label() const override { return "number"; }

  float random_number;

  NumberMsg(uint32_t sequence_number, uint32_t ack_sequence_number
       , float random_number)
    : Message(sequence_number, ack_sequence_number)
    , random_number(random_number)
  {}

  NumberMsg(std::istream& is) : Message(is) {
    is >> random_number;
  }

  void to_stream(std::ostream& os) const override {
    os << random_number;
  }
};

//------------------------------------------------------------------------------
struct StatusMsg : Message {
  std::string label() const override { return "status"; }

  LeaderStatus leader_status;

  StatusMsg(uint32_t sequence_number, uint32_t ack_sequence_number
       , LeaderStatus leader_status)
    : Message(sequence_number, ack_sequence_number)
    , leader_status(leader_status)
  {}

  StatusMsg(std::istream& is) : Message(is) {
    is >> leader_status;
  }

  void to_stream(std::ostream& os) const override {
    os << leader_status;
  }
};

//------------------------------------------------------------------------------
struct ResultMsg : Message {
  std::string label() const override { return "result"; }

  LeaderStatus leader_status;

  ResultMsg(uint32_t sequence_number, uint32_t ack_sequence_number
       , LeaderStatus leader_status)
    : Message(sequence_number, ack_sequence_number)
    , leader_status(leader_status)
  {}

  ResultMsg(std::istream& is) : Message(is) {
    is >> leader_status;
  }

  void to_stream(std::ostream& os) const override {
    os << leader_status;
  }
};

//------------------------------------------------------------------------------
template< typename PingHandler
        , typename StartHandler
        , typename NumberHandler
        , typename StatusHandler
        , typename ResultHandler
        >
void dispatch_message( std::istream& is
                     , const PingHandler&    ping_handler
                     , const StartHandler&   start_handler
                     , const NumberHandler&  random_number_handler
                     , const StatusHandler&  status_handler
                     , const ResultHandler&  result_handler) {
  using namespace std;

  string label;
  is >> label;

  if (label == "ping") {
    ping_handler(PingMsg(is));
  }
  else if (label == "start") {
    start_handler(StartMsg(is));
  }
  else if (label == "number") {
    random_number_handler(NumberMsg(is));
  }
  else if (label == "status") {
    status_handler(StatusMsg(is));
  }
  else if (label == "result") {
    result_handler(ResultMsg(is));
  }
  else {
    throw runtime_error("unrecognized message label");
  }
}

//------------------------------------------------------------------------------

#endif // ifndef __PROTOCOL_H__
