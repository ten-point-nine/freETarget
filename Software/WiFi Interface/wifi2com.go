package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"time"

	"github.com/jacobsa/go-serial/serial"
)

func bridge2Com(msg string, port io.ReadWriteCloser) {
	_, err := port.Write([]byte(msg))
	if err != nil {
		log.Fatalf("port.Write: %v", err)
	}
}
func handleClient(conn net.Conn, port io.ReadWriteCloser) {
	r := bufio.NewReader(conn)
	for {
		line, err := r.ReadBytes(byte('\n'))
		//line , err := r.ReadLine()
		switch err {
		case nil:
			break
		case io.EOF:
		default:
			fmt.Println("Error: ", err)
		}
		firstChar := line[0]
		msg := line
		if string(firstChar) == "{" {
			println("This is a shot of length :", len(line))
			println("Data :", string(msg))
			//r.Discard(2)
			bridge2Com(string(msg), port)
		} else if string(firstChar) == "f" {
			println("This is the startup message of length :", len(line))
			println("Data :", string(msg))
			//r.Discard(1)
			bridge2Com(string(msg), port)
		} else {
			println("This is the rest message of length :", len(line))
			println("Data :", string(msg))
			//r.Discard(len(line) - 1)

		}

	}
}
func main() {
	// Set up options.
	comPort := flag.String("port", "COM9", "PC Software COM Port")
	flag.Parse()
	println("COM Port Name : ", *comPort)
	options := serial.OpenOptions{
		PortName:   "COM9",
		BaudRate:   115200,
		DataBits:   7,
		StopBits:   1,
		ParityMode: serial.PARITY_EVEN,
		//MinimumReadSize: 4,
	}
	// tcp address for target
	servAddr := "192.168.10.9:1090"

	// Open the port.
	port, err := serial.Open(options)
	if err != nil {
		log.Fatalf("serial.Open: %v", err)
		os.Exit(1)
	}

	// Make sure to close it later.
	defer port.Close()

	time.Sleep(3 * time.Second)

	tcpAddr, err := net.ResolveTCPAddr("tcp", servAddr)
	if err != nil {
		println("resolver failed : ", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		println("dial failed : ", err.Error())
		os.Exit(1)
	}
	handleClient(conn, port)
}
