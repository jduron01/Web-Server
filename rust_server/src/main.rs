use std::net::{TcpListener, TcpStream, Shutdown};
use httparse::{Request, Status, EMPTY_HEADER};
use std::io::{ErrorKind, Read, Write};
use std::fs::{read, File};
use chrono::format::{DelayedFormat, StrftimeItems};
use chrono::Utc;
use std::path::Path;
use mime_guess::{from_path, Mime};

fn main() {
    let listener: TcpListener = TcpListener::bind("127.0.0.1:8080").unwrap();

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

    let response: String = create_response(&buffer[..bytes_read]);

    if let Err(e) = stream.write_all(response.as_bytes()) {
        eprintln!("Error sending response to client: {}", e);
    }

    if let Err(e) = stream.flush() {
        eprintln!("Error flushing stream: {}", e);
    }

    if let Err(e) = stream.shutdown(Shutdown::Both) {
        if e.kind() != std::io::ErrorKind::NotConnected {
            eprintln!("Error closing connection: {}", e);
        }
    }
}

fn create_response(buffer: &[u8]) -> String {
    let mut headers = [EMPTY_HEADER; 16];
    let mut request = Request::new(&mut headers);
    let version: &str = "HTTP/1.1";

    match request.parse(buffer) {
        Ok(status) if status.is_complete() => {
            if request.method.is_none() || request.path.is_none() {
                return create_error_response(version, "400 Bad Request");
            }

            let method: &str = request.method.unwrap();
            let path: &str = request.path.unwrap();
            let file_name: String = format!("public{path}");

            if method == "GET" {
                return handle_get_request(file_name.as_str(), version);
            } else if method == "POST" {
                let index: Status<usize> = request.parse(buffer).unwrap();
                let bytes: &[u8] = match index {
                    Status::Complete(n) => {
                        &buffer[n..]
                    } Status::Partial => {
                        return create_error_response(version, "400 Bad Request");
                    }
                };
                let body: &str = match str::from_utf8(bytes) {
                    Ok(s) => {
                        s
                    } Err(_) => {
                        return create_error_response(version, "400 Bad Request");
                    }
                };

                if body.len() > 10000 {
                    return create_error_response(version, "413 Payload Too Large");
                }

                return handle_post_request(file_name.as_str(), version, body);
            } else {
                return create_error_response(version, "501 Not Implemented");
            }
        } _ => {
            return create_error_response(version, "400 Bad Request");
        }
    }
}

fn handle_get_request(file_name: &str, version: &str) -> String {
    match read(file_name) {
        Ok(contents) => {
            let date: DelayedFormat<StrftimeItems> = Utc::now().format("%Y-%m-%d %H:%M:%S %Z");
            let content_type: Mime = from_path(Path::new(file_name)).first_or_octet_stream();
            let response = format!(
                "{} 200 OK\r\nDate: {}\r\nContent-Length: {}\r\nContent-Type: {}\r\n\r\n{}",
                version, date, contents.len(), content_type, String::from_utf8_lossy(&contents)
            );

            return response;
        } Err(e) if e.kind() == ErrorKind::NotFound => {
            return create_error_response(version, "404 Not Found");
        } Err(e) => {
            eprintln!("Error reading from {}: {}", file_name, e);
            return create_error_response(version, "500 Internal Server Error");
        }
    }
}

fn handle_post_request(file_name: &str, version: &str, body: &str) -> String {
    match File::create(file_name) {
        Ok(mut file) => {
            if let Err(e) = file.write_all(body.as_bytes()) {
                eprintln!("Could not write all of the data to {}: {}", file_name, e);
                return create_error_response(version, "500 Internal Server Error");
            }

            let date: DelayedFormat<StrftimeItems> = Utc::now().format("%Y-%m-%d %H:%M:%S %Z");
            let content_type: Mime = from_path(Path::new(file_name)).first_or_octet_stream();
            let response = format!(
                "{} 200 OK\r\nDate: {}\r\nContent-Length: {}\r\nContent-Type: {}\r\n\r\n{}",
                version, date, body.len(), content_type, body
            );

            return response;
        } Err(e) => {
            eprintln!("Could not open {}: {}", file_name, e);
            return create_error_response(version, "500 Internal Server Error");
        }
    }
}

fn create_error_response(version: &str, status: &str) -> String {
    return format!("{} {}\r\n\r\n", version, status);
}