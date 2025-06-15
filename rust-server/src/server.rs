use std::fs::{read, File};
use std::io::{ErrorKind, Write};
use std::path::Path;
use httparse::{Request, Status, EMPTY_HEADER};
use chrono::Utc;
use chrono::format::{DelayedFormat, StrftimeItems};
use mime_guess::{from_path, Mime};

pub fn create_response(buffer: &[u8]) -> String {
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
                    Ok(string) => {
                        string
                    } Err(e) => {
                        eprintln!("Error converting from UTF-8 to a string: {}", e);
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

pub fn handle_get_request(file_name: &str, version: &str) -> String {
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

pub fn handle_post_request(file_name: &str, version: &str, body: &str) -> String {
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

pub fn create_error_response(version: &str, status: &str) -> String {
    return format!("{} {}\r\n\r\n", version, status);
}