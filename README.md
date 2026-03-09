# TinyLink URL Shortener

A high-performance URL shortener built in C++ using the Crow web framework.

**🚀 Test it Live!** [https://tinylink-l69h.onrender.com/](https://tinylink-l69h.onrender.com/)

---

## What Changed:
✅ Added `crow_all.h` dependency for blazing fast C++ HTTP processing
✅ Built an elegant, glassmorphic UI using Tailwind CSS
✅ Implemented CORS handling for browser safety
✅ Prepared for cloud deployment with Docker and dynamic PORT binding
✅ Uses `InMemoryUrlRepository` following SOLID principles

## Key Features
- **URL shortening:** Generate random 6-character Base62 codes.
- **Web interface:** Beautiful HTML frontend served directly by C++.
- **REST API:** Clean JSON endpoints for programmatic access.
- **Redirect functionality:** Deep HTTP 302 backend logic.
- **Input validation & Error handling.**

---

## 💻 Interview Questions on URL Shortener

### System Design Questions:
**1. Design a URL Shortening Service like Bitly**
- **Requirements:** Shorten URLs, redirect, handle high traffic
- **Scale:** 100M URLs/day, 10B redirects/day
- **Components:** Load balancer, web servers, DB, cache, analytics

**2. Database Schema Design**
- **Tables:** `urls` (id, long_url, short_code, created_at, user_id, expires_at)
- **Indexes:** `short_code` (unique), `created_at`
- **Partitioning:** By date for old URLs

**3. Scalability Challenges**
- **Hot Keys:** Popular URLs cause cache misses
- **Write Heavy:** 100M new URLs/day
- **Read Heavy:** 10B redirects/day (100:1 read:write ratio)
- **DDoS:** Malicious redirects

**4. Caching Strategy**
- **Multi-level:** Redis → Memcached → DB
- **Cache Aside:** Read from cache, write to DB + invalidate cache
- **TTL:** Popular URLs stay longer

### Low Level Design (LLD) Questions:
**1. URL Generation Algorithm**
- Use Base62 (`[0-9][a-z][A-Z]`) to generate 6-7 character strings, supporting billions of combinations.

**2. Collision Handling**
- Use database constraints (unique `short_code`)
- Retry with different ID if collision occurs
- Pre-generate codes in batches to ensure uniqueness

**3. Rate Limiting**
- **Per user:** 100 URLs/hour
- **Per IP:** 1000 requests/hour
- Use Redis for sliding window counters with TTL

**4. Analytics & Monitoring**
- **Track:** clicks, referrers, geolocation
- **Metrics:** response time, error rates
- **Alerts:** high latency, DB connection issues

**5. Security Considerations**
- Input validation (URL format, length)
- Prevent XSS in redirects
- Rate limiting for abuse prevention
- HTTPS mandatory

### Follow-up Questions:
- How would you handle custom short codes? *(We currently check for alias availability before saving)*
- What if a URL expires?
- How to implement user accounts?
- Mobile app integration?
- API versioning strategy?

---

## Running the Application Locally
**Compile & Start:**
```bash
g++ url.shortner.cpp -I include -o url.shortner.exe -D_WIN32_WINNT=0x0601 -lws2_32 -lmswsock -pthread
.\url.shortner.exe
```
**Access:**
- Frontend: `http://localhost:8080/`
- API Shorten: `http://localhost:8080/shorten`
- Analytics: `http://localhost:8080/analytics/<short_hash>`
