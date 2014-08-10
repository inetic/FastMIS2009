#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

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
  return os << msg.sequence_number;
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
struct Ping : MessageTemplate<Ping> {
  std::string label() const override { return "ping"; }

  Ping(uint32_t sequence_number, uint32_t ack_sequence_number)
    : MessageTemplate(sequence_number, ack_sequence_number) {}

  Ping(std::istream& is) : MessageTemplate(is) {}
};

inline std::ostream& operator<<(std::ostream& os, const Ping& ping) {
  return os << static_cast<const Message&>(ping);
}

//------------------------------------------------------------------------------
struct Start : MessageTemplate<Start> {
  std::string label() const override { return "start"; }

  float random_number;

  Start(uint32_t sequence_number, uint32_t ack_sequence_number
       , float random_number)
    : MessageTemplate(sequence_number, ack_sequence_number)
    , random_number(random_number)
  {}

  Start(std::istream& is) : MessageTemplate(is) {
    is >> random_number;
  }
};

inline std::ostream& operator<<(std::ostream& os, const Start& start) {
  return os << static_cast<const Message&>(start) << " " << start.random_number;
}

//------------------------------------------------------------------------------
template< typename PingHandler
        , typename StartHandler
        >
void dispatch_message( std::istream& is
                     , const PingHandler&  ping_handler
                     , const StartHandler& start_handler) {
  using namespace std;

  string label;
  is >> label;

  if (label == "ping") {
    ping_handler(Ping(is));
  }
  else if (label == "start") {
    start_handler(Start(is));
  }
  else {
    throw runtime_error("unrecognized message label");
  }
}

//------------------------------------------------------------------------------

#endif // ifndef __PROTOCOL_H__
