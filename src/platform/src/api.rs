use hyper::header::{CONTENT_LENGTH, CONTENT_TYPE};
use hyper::{Request, Response, Body};

type GenericError = Box<dyn std::error::Error + Send + Sync>;
type Result<T> = std::result::Result<T, GenericError>;


pub fn mpi_get(_: Request<Body>) -> Result<Response<Body>> {
    let body = "MPI_GET";
    Ok(Response::builder()
        .header(CONTENT_LENGTH, body.len() as u64)
        .header(CONTENT_TYPE, "text/plain")
        .body(Body::from(body))
        .expect("Failed to construct the response"))
}

pub fn mpi_set(_: Request<Body>) -> Result<Response<Body>> {
    let body = "MPI_SET";
    Ok(Response::builder()
        .header(CONTENT_LENGTH, body.len() as u64)
        .header(CONTENT_TYPE, "text/plain")
        .body(Body::from(body))
        .expect("Failed to construct the response"))
}