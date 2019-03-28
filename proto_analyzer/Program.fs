// Learn more about F# at http://fsharp.org

open System
open System.IO
open System.Threading
open System.IO.Ports
open A4Cmds
open Microsoft.FSharp.Text.StructuredFormat
open Microsoft.FSharp.Text.StructuredFormat.LayoutOps
open Newtonsoft.Json

let blockL sep_l sep_r (xs: Layout list) =
    aboveListL [
    sepL sep_l @@-
        List.reduce (@@) xs
    sepL sep_r
    ]

let pairL k v =
    wordL k ^^ sepL ":" ^^ objL v

let byteL = sprintf "0x%02x"
let bytesL xs = sprintf "%A" <| List.map byteL xs

let fmt l = Display.layout_to_string FormatOptions.Default l

[<StructuredFormatDisplay("{Str}")>]
type CommunicationRecord = 
    {
        cmd:        byte;
        parameters: byte list;
        response:   byte list;
        send_time:  DateTime;
        recv_time:  DateTime;
    }
    with 
    [<JsonIgnore>]
    member x.Str = 
        let layout = blockL "{" "}" [
            pairL "cmd" (byteL x.cmd)
            pairL "params" (bytesL x.parameters)
            pairL "response" (bytesL x.response)
        ]
        fmt layout

type CommunicationRecords = CommunicationRecord list

let communicate (com: SerialPort) (input: int list) : CommunicationRecord = 
    match input with
    | [] -> failwith "empty input"
    | cmd :: _ when List.contains cmd A4_BAD_COMMANDS -> failwith "bad command"
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
    let rec __read_exhaustive data = 
        let read_byte = 
            try com.ReadByte() |> byte |> Some
            with | :? TimeoutException -> None
        match read_byte with
        | Some x -> __read_exhaustive(x :: data)
        | None ->
          {
            cmd = byte(cmd)
            parameters = List.map byte parameters
            send_time = send_time
            recv_time = DateTime.Now
            response = List.rev data
          }
    __read_exhaustive []

let rng = new System.Random()
let rand() = rng.Next(0, 128)
type GenT =
| Random
| Fixed of data: int list * editing: int

let synthesize_payload len =
    [for _ in 1..len -> rand()]

let rec probe com dat cmd len r =
    let len = max 0 <| min len 6
    let cmd = max 0 <| min cmd 127
    let r = match r with
            | Fixed(dat, _) when List.length dat <> len -> Fixed(synthesize_payload len, 0)
            | Fixed(dat, i) -> Fixed(dat, max 0 <| min i (List.length dat - 1))
            | _ -> r

    let request = cmd :: match r with   
                         | Random -> synthesize_payload len
                         | Fixed(dat, _) -> dat

    let mk_comm() = 
        try
            Console.SetCursorPosition(90, 4)
            printf "= BUSY ="
            let record = communicate com request
            record :: dat
        with
        | ex -> 
            printfn "cmd=0x%02x: %s" cmd ex.Message
            dat

    Console.Clear()
    let history = List.truncate 5 dat
    history |> List.iter (printfn "%A")
    Console.SetCursorPosition(90, 0)
    printf "Current cmd = 0x%02x; Len = %A" cmd len
    Console.SetCursorPosition(90, 1)
    printf "Current dat = "
    let ith_pos i = 104 + i * 3
    request |> List.iteri (fun i x -> Console.SetCursorPosition(ith_pos i, 1); printf "%02x" x)

    match r with
    | Fixed(_, i) ->
      Console.SetCursorPosition(2 + ith_pos i+2, 2)
      printf("^")
    | _ -> ()

    let probe' = probe com dat
    let k = Console.ReadKey().Key

    Thread.Sleep(10)

    let plus  i i' x = if i <> i' then x else x+1
    let minus i i' x = if i <> i' then x else x-1

    match k, r with
    | ConsoleKey.Escape, _        -> dat
    | ConsoleKey.Enter, _         -> probe com (mk_comm()) cmd len r
    | ConsoleKey.PageDown, _      -> probe' (cmd+1) len r
    | ConsoleKey.PageUp, _        -> probe' (cmd-1) len r
    | ConsoleKey.Home, _          -> probe' cmd (len-1) r
    | ConsoleKey.End, _           -> probe' cmd (len+1) r
    | ConsoleKey.Spacebar, Random -> probe' cmd len <| Fixed(synthesize_payload len, 0)
    | ConsoleKey.Spacebar, Fixed _-> probe' cmd len Random
    | _, Fixed(dat, i) ->
      let dat, i = 
          match k with
          | ConsoleKey.UpArrow    -> List.mapi (plus i)  dat, i
          | ConsoleKey.DownArrow  -> List.mapi (minus i) dat, i
          | ConsoleKey.LeftArrow  -> dat, i-1
          | ConsoleKey.RightArrow -> dat, i+1
          | _                     -> dat, i
      probe' cmd len <| Fixed(dat, i)
    | _                      -> probe' cmd len r
               
    
[<EntryPoint>]
let main argv =
    use com = new SerialPort("COM3", 250000)
    com.ReadTimeout <- 100
    com.Open()

    let results = 
        probe com [] 0x64 1 Random
        |> List.map JsonConvert.SerializeObject

    File.AppendAllLines("records.json", results)
    

    0 // return an integer exit code
