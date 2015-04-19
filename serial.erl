-module(serial).
-author('Oleksandr Novychenko <novychenko@evologics.de>').

-export([ start/0, start/1, init/1, loop/2 ]).

-include("serial.hrl").

start() ->
    start([]).

start(Options) ->
    ChildPid = spawn_link(serial, init, [ self() ]),
    setopt(ChildPid, Options),
    ChildPid.

setopt(_ChildPid, []) -> done;
setopt(ChildPid, [ Opt | Opts ]) ->
    ChildPid ! Opt,
    setopt(ChildPid, Opts).

init(ParentPid) ->
    process_flag(trap_exit, true),
    Path = "./serial",
    Port = open_port({ spawn, Path }, [ binary, { packet, 1 }, overlapped_io ]),
    loop(ParentPid, Port).

loop(ParentPid, Port) ->
    receive
	{ Port, { data, Data } } ->
        [ Cmd | Payload ] = binary_to_list(Data),
        ParentPid ! { data, Cmd, Payload },
        io:format("RECV ~p: ~p~n", [ Cmd, Payload ]),
	    loop(ParentPid, Port);

	{ send, Data } ->
	    sendcmd(Port, [ ?SEND, Data ]),
	    loop(ParentPid, Port);

	{ enumerate } ->
	    sendcmd(Port, [ ?ENUMERATE ]),
	    loop(ParentPid, Port);

	{ baudrate, Baudrate } -> % 300..115200
	    sendcmd(Port, [ ?BAUDRATE, integer_to_list(Baudrate) ]),
	    loop(ParentPid, Port);

	{ flowcontrol, FlowControl } -> % 0 - none, 1 - RTS/CTS
	    sendcmd(Port, [ ?FLOWCONTROL, FlowControl ]),
	    loop(ParentPid, Port);

	{ databits, DataBits } -> % 5..8
	    sendcmd(Port, [ ?DATABITS, DataBits ]),
	    loop(ParentPid, Port);

	{ parity, Parity } -> % "none", "odd" or "even"
	    sendcmd(Port, [ ?PARITY, Parity ]),
	    loop(ParentPid, Port);

	{ stopbits, StopBits } -> % 1..2
	    sendcmd(Port, [ ?STOPBITS, StopBits ]),
	    loop(ParentPid, Port);

	{ open, Name } ->
	    sendcmd(Port, [ ?OPEN, Name ]),
	    loop(ParentPid, Port);

	{ close } ->
	    sendcmd(Port, [ ?CLOSE ]),
	    loop(ParentPid, Port);

	stop ->
	    stopped;

	{ 'EXIT', Port, Why } ->
	    io:format("Port exited with reason ~w~n", [ Why ]),
	    exit(Why);

	{ 'EXIT', Linked, Why } ->
	    io:format("Linked ~w exited with reason ~w~n", [ Linked, Why ]),
	    exit(Why);

	OtherError ->
	    io:format("Received unknown message ~w~n", [ OtherError ]),
	    loop(ParentPid, Port)

    end.

sendcmd(Port, Message) ->
    Port ! { self(), { command, Message } }.
