on message event
- if message type is welcome
    - if state == opened
        quit welcome thread
        change state to established
    - else
        raise protocol violation
- if message type is abort
    - if state == opened
        quit welcome thread
        change state to refused
      else
        change state to aborted
- if message type is pong
    if state == established
      send reset to heartbeat thread
    else
      raise an error ???
- if message type is published
    if state == established
      quit wait for reply thread
      
