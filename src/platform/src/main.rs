
use std::{fs, path::Path};
use hyper::service::{make_service_fn, service_fn};
use hyper::{Body, Method, Request, Response, Server, StatusCode};
use hyperlocal::UnixServerExt;

type GenericError = Box<dyn std::error::Error + Send + Sync>;
type Result<T> = std::result::Result<T, GenericError>;

mod api;

static NOTFOUND: &[u8] = b"Not Found";

async fn route_request(
    req: Request<Body>
) -> Result<Response<Body>> {
    match (req.method(), req.uri().path()) {
        (&Method::GET, "/mpi") => api::mpi_get(req),
        (&Method::POST, "/mpi") => api::mpi_set(req),
        _ => {
            // Return 404 not found response.
            Ok(Response::builder()
                .status(StatusCode::NOT_FOUND)
                .body(NOTFOUND.into())
                .unwrap())
        }
    }
}

fn basic_handler(_: Request<Body>) -> Response<Body> {
    let body = "Hello World";
    Response::builder()
        .header(CONTENT_LENGTH, body.len() as u64)
        .header(CONTENT_TYPE, "text/plain")
        .body(Body::from(body))
        .expect("Failed to construct the response")
}

fn router_service() -> Result<RouterService> {
    let router = RouterBuilder::new()
        .add(Route::get("/greet").using(basic_handler))
        .build();

    Ok(RouterService::new(router))
}

#[tokio::main]
async fn main() -> Result<()> {
    let path = Path::new("/tmp/hyperlocal.sock");

    if path.exists() {
        fs::remove_file(path)?;
    }

    // let make_service = make_service_fn(|_| async {
    //     Ok::<_, GenericError>(service_fn(move |req| {
    //         route_request(req)
    //     }))
    // });

    // Server::bind_unix(path)?.serve(make_service).await?;
    Server::bind_unix(path)?.serve(router_service).await?;

    Ok(())
}