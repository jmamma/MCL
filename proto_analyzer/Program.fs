// Learn more about F# at http://fsharp.org

open System
open System.IO.Ports

type CommunicationRecord = 
    {
        cmd:        byte;
        parameters: byte list;
        send_time:  DateTime;
        response:   byte list;
        recv_time:  DateTime;
    }

let communicate (com: SerialPort) (input: int list) : CommunicationRecord = 
    match input with
    | [] -> failwith "empty input"
    | cmd :: parameters ->
    let request = 
        [
            // begin sysex
            [ 0xf0 ]
            // A4 header
            [ 0x00; 0x20; 0x3c; 0x06; 0x00 ]
            // command
            [ cmd ]
            // a4 proto. ver.
            [ 0x01; 0x01 ]
            // the rest of input
            parameters
            // a4 proto. endframe
            [ 0x00; 0x00; 0x00; 0x05]
            // end sysex
            [ 0xf7 ]
        ]
        |> List.concat
        |> List.map byte
        |> List.toArray
    let send_time = DateTime.Now
    com.Write(request, 0, Array.length request)

    // read until first timeout
    let read_exhaustive () =
        let rec __read_exhaustive data = 
            try
                let x = com.ReadByte() |> byte
                __read_exhaustive(x :: data)
            with
            | :? TimeoutException -> List.rev data
        __read_exhaustive []

    {
        cmd = byte(cmd)
        parameters = List.map byte parameters
        send_time = send_time
        recv_time = DateTime.Now
        response = read_exhaustive()
    }

[<EntryPoint>]
let main argv =
    use com = new SerialPort("COM3", 250000)
    com.ReadTimeout <- 100
    
    0 // return an integer exit code
