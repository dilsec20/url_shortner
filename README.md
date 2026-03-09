# TinyLink URL Shortener

A high-performance URL shortener built in C++ using the Crow web framework.

**🚀 Test it Live!** [https://tinylink-l69h.onrender.com/](https://tinylink-l69h.onrender.com/)

## Features
- **Fast:** Written in C++ for maximum performance.
- **RESTful API:** Clean JSON endpoints for creating and resolving URLs.
- **Custom Aliases:** Choose your own short links (e.g., `tinylink.co/myvideo`).
- **Analytics:** Built-in click tracking.
- **Modern UI:** Comes with a beautiful glassmorphic frontend out of the box.

## Deployment
Docker ready! Just push to a cloud provider like Render via the included `Dockerfile`!

---

## Low-Level Design (LLD) & System Design Interview Context
This project serves as an excellent reference implementation for the classic "Design a URL Shortener" (TinyURL) system design interview question.

### 1. How do you generate the short alias? (The Core Problem)
**Interviewer:** "How do we convert a long URL into a short 6-7 character string?"
**Our Implementation:** We use **Base62 Encoding** (`[0-9][a-z][A-Z]`).
- 62 characters ^ 6 length = ~56.8 Billion unique combinations.
- We generate a random 6-character Base62 string and check our memory hash map for collisions. 
- *Note for scaling:* In a real distributed system (System Design), we would use a **Ticket Server** or **ZooKeeper/Snowflake** ID generator to ensure uniqueness across multiple servers before Base62 encoding the sequential ID, rather than relying on random generation which introduces hash collisions at scale.

### 2. How do you handle Custom Aliases?
**Interviewer:** "What if users want their own alias like `/mybrand`?"
**Our Implementation:** Our `UrlShortenerService` explicitly handles this by first checking if the custom alias exists in the `IUrlRepository`. If it exists, we return an error (HTTP 400). If not, we reserve it immediately. This requires a fast read-before-write check.

### 3. What architecture principles did you follow?
**Interviewer:** "How did you structure the codebase to be maintainable?"
**Our Implementation:** We heavily utilized **SOLID Principles**, specifically the **Dependency Inversion Principle (DIP)**:
- We defined an interface `IUrlRepository`.
- The business logic (`UrlShortenerService`) depends only on the interface, NOT the database.
- We implemented a concrete `InMemoryUrlRepository` (using C++ `unordered_map`). 
- If we want to scale to PostgreSQL or Redis later, we simply create a `PostgresUrlRepository` that inherits `IUrlRepository` and inject it into the service without modifying *any* core logic!

### 4. How do URL redirects actually work?
**Interviewer:** "What happens at the HTTP level when someone clicks a short link?"
**Our Implementation:** The Crow HTTP server receives a `GET` request on the `/<string>` route. We look up the long URL in our map. If found, instead of returning HTML, we return an **HTTP 302 (Found)** status code with the `Location: <LongUrl>` header. The user's browser automatically handles the jump!

### 5. Why C++ and Crow?
While Node.js or Python is common for this, C++ offers near-zero overhead thread management and blazing fast raw HTTP throughput using the asynchronous `Asio` library underlying Crow. This makes it ideal for the read-heavy nature of a URL shortener (reads > writes by 100:1).
