Control Channel has state and controls the websocket endpoint, eg. connect and send message methods.

State Machine:
==============

PENDING (initial state)
  WebSocket is about to open.
  -> Action: Connect
     On success the state changes to OPENED, CLOSED or may be FAILED,
     but we do not change the state in this action!

OPENED (websocketpp callback on_open)
  WebSocket is connected.
  -> Action: Send the hello message
     We should start a n seconds timer for the response.
     On successful send we hopefully receive a message of type welcome or abort within the defined timeout period.
     But what if send fails (technical exception) ???

ESTABLISHED (transition by the welcome message handler)
  WebSocket connection is authorized (hello and welcome message exchanged).
  -> Action: Start the heartbeat (Ping/Pong) 

REFUSED (transition by the abort message handler during state OPENED)
  WebSocket connection is refused by the server, we received an abort message
  with reason NO_SUCH_REALM.
  -> Action: ???

ABORTED (transition by the abort message handler during state ESTABLISHED)
  WebSocket received an abort message. The connection will/should be closed soon.
  Start a force close on timeout routine.
  -> Action: ??? (Retry???)
     - if state is ESTABLISHED stop the hearbeat first

EXPIRED (transition by the heartbeat routine)
  WebSocket connection is expired becuase we didn't received a pong message within defined timeout period.
  -> Action: ??? (Retry???)

UNACKNOWLEDGED (transition by a expired request/reply)
  WebSocket did not received a reply to a request (call or publish message)
  -> Action: ???

CLOSED (websocketpp callback on_close)
  WebSocket connection is closed.
  -> Action: Stop 
     - if state is ESTABLISHED stop the hearbeat first!
     - else if state ABORTED stop the force close routine.

FAILED (websocketpp callback on_fail)
  WebSocket connection is failed.
  -> Action: Stop




Message Receiver (websocketpp on message):
==========================================

  - parse message
  - on error ???
  - on success
    -> if welcome   -> handle welcome message
    -> if abort     -> handle abort message
    -> if error     -> handle error message
    -> if pong      -> handle pong message
    -> if call      -> handle call message
    -> if published -> handle published message
    -> if hello     -> not supported !!! abort ???
    -> if ping      -> not supported !!! abort ???
    -> if result    -> not supported !!! abort ???
    -> if publish   -> not supported !!! abort ???

Message Handler:
================

handle welcome message
  - if state is OPENED
    -> Action: Change state to ESTABLISHED
  - else
    -> Action: We have a protocol violation!


handle abort message
  - if state is OPENED
    -> Action: ?
       Check why server aborted our connection
       - if error is ERR_NO_SUCH_REALM 
         We should better leave? Eg. action -> REFUSED ?
       - else
         Should we retry?
       Note: The server closes the connection! But should we close the connection after a certain time, too?
  - if state is ESTABLISHED
    -> Action: ???
       Something bad happend, what should we do???
  - else
    -> Action: ???
       Protocol violation ? May be this else path never happens! 

handle pong message
  - if state != ESTABLISHED
      We have a protocol violation ???
  - else
      Reset heartbeat because we're online
