use std::net::{TcpListener, TcpStream, Shutdown};
use std::io::{Read, Write, ErrorKind};
mod server;

fn main() {
    let listener: TcpListener = TcpListener::bind("127.0.0.1:8080").unwrap();
    println!("Server is listening on port 8080...");

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                handle_client(stream);
            } Err(e) => {
                eprintln!("Connection failed: {}", e);
            }
        }
    }
}

fn handle_client(mut stream: TcpStream) {
    let mut buffer: [u8; 1024] = [0; 1024];
    let bytes_read: usize;

    match stream.read(&mut buffer) {
        Ok(n) => {
            bytes_read = n;
        } Err(e) => {
            eprintln!("Could not receive data from client: {}", e);

            if let Err(e) = stream.shutdown(Shutdown::Both) {
                eprintln!("Error closing connection: {}", e);
            }

            return;
        }
    }

    let response: String = server::create_response(&buffer[..bytes_read]);

    if let Err(e) = stream.write_all(response.as_bytes()) {
        eprintln!("Error sending response to client: {}", e);
    }

    if let Err(e) = stream.flush() {
        eprintln!("Error flushing stream: {}", e);
    }

    if let Err(e) = stream.shutdown(Shutdown::Both) {
        if e.kind() != ErrorKind::NotConnected {
            eprintln!("Error closing connection: {}", e);
        }
    }
}