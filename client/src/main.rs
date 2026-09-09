use clap::{Parser, Subcommand};
use serde::{Deserialize, Serialize};
use std::fs;
use std::io::{self, Write};
use std::os::unix::fs::PermissionsExt;
use std::path::PathBuf;

#[derive(Parser)]
#[command(name = "nimbus", about = "Nimbus API client")]
struct Cli {
    #[arg(long, default_value = "http://127.0.0.1:8080")]
    api: String,
    #[command(subcommand)]
    cmd: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Health,
    Register,
    Login,
    Whoami,
    Logout,
    Add {
        #[arg(long)]
        title: String,
        #[arg(long)]
        body: String,
    },
    List,
    Delete {
        #[arg(long)]
        id: i64,
    },
}

#[derive(Deserialize)]
struct Health {
    status: String,
    service: String,
}

#[derive(Serialize)]
struct Creds {
    username: String,
    password: String,
}

#[derive(Deserialize)]
struct LoginResp {
    token: String,
    role: String,
}

#[derive(Deserialize)]
struct Me {
    id: i64,
    username: String,
    role: String,
}

#[derive(Serialize)]
struct NewRecord {
    title: String,
    body: String,
}

#[derive(Deserialize)]
struct Record {
    id: i64,
    title: String,
    body: String,
}

#[derive(Deserialize)]
struct Created {
    id: i64,
}

fn session_path() -> PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".into());
    PathBuf::from(home).join(".local/share/nimbus/session")
}

fn save_token(token: &str) -> io::Result<()> {
    let path = session_path();
    if let Some(dir) = path.parent() {
        fs::create_dir_all(dir)?;
    }
    fs::write(&path, token)?;
    fs::set_permissions(&path, fs::Permissions::from_mode(0o600))?;
    Ok(())
}

fn load_token() -> io::Result<String> {
    fs::read_to_string(session_path()).map(|s| s.trim().to_string())
}

fn prompt(label: &str) -> io::Result<String> {
    print!("{label}");
    io::stdout().flush()?;
    let mut s = String::new();
    io::stdin().read_line(&mut s)?;
    Ok(s.trim().to_string())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();
    let base = cli.api.trim_end_matches('/').to_string();
    let http = reqwest::blocking::Client::new();

    match cli.cmd {
        Commands::Health => {
            let h: Health = http
            .get(format!("{base}/health"))
            .send()?
            .error_for_status()?
            .json()?;
            println!("{} ({})", h.status, h.service);
        }
        Commands::Register => {
            let username = prompt("username: ")?;
            let password = prompt("password: ")?;
            let res = http
            .post(format!("{base}/api/v1/auth/register"))
            .json(&Creds { username, password })
            .send()?;
            if res.status() == 201 {
                println!("registered");
            } else {
                println!("error {}: {}", res.status(), res.text()?);
            }
        }
        Commands::Login => {
            let username = prompt("username: ")?;
            let password = prompt("password: ")?;
            let res = http
            .post(format!("{base}/api/v1/auth/login"))
            .json(&Creds { username, password })
            .send()?
            .error_for_status()?;
            let body: LoginResp = res.json()?;
            save_token(&body.token)?;
            println!("logged in as {}", body.role);
        }
        Commands::Whoami => {
            let token = load_token()?;
            let me: Me = http
            .get(format!("{base}/api/v1/me"))
            .header("Authorization", format!("Bearer {token}"))
            .send()?
            .error_for_status()?
            .json()?;
            println!("{} ({}) id={}", me.username, me.role, me.id);
        }
        Commands::Logout => {
            let _ = fs::remove_file(session_path());
            println!("logged out");
        }
        Commands::Add { title, body } => {
            let token = load_token()?;
            let created: Created = http
            .post(format!("{base}/api/v1/records"))
            .header("Authorization", format!("Bearer {token}"))
            .json(&NewRecord { title, body })
            .send()?
            .error_for_status()?
            .json()?;
            println!("created id={}", created.id);
        }
        Commands::List => {
            let token = load_token()?;
            let rows: Vec<Record> = http
            .get(format!("{base}/api/v1/records"))
            .header("Authorization", format!("Bearer {token}"))
            .send()?
            .error_for_status()?
            .json()?;
            for r in rows {
                println!("[{}] {} — {}", r.id, r.title, r.body);
            }
        }
        Commands::Delete { id } => {
            let token = load_token()?;
            let res = http
            .delete(format!("{base}/api/v1/records/{id}"))
            .header("Authorization", format!("Bearer {token}"))
            .send()?;
            if res.status() == 204 {
                println!("deleted");
            } else {
                println!("error {}: {}", res.status(), res.text()?);
            }
        }
    }
    Ok(())
}
