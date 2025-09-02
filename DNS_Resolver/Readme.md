# DNS Resolver Assignment

This project implements both iterative and recursive DNS resolution. It is designed to resolve domain names by first querying root servers and then following the DNS hierarchy until an answer is obtained. The code uses the Python **dnspython** library to construct, send, and process DNS queries.

---

## Assignment Features

**Implemented:**
- **Iterative DNS Lookup:**  
  - Starts with a predefined set of root DNS servers.
  - Queries a server for an A record.
  - If no answer is found, extracts NS records from the authority section.
  - Resolves NS hostnames to IP addresses and continues the lookup through TLD and authoritative servers.
  
- **Recursive DNS Lookup:**  
  - Leverages the system's default DNS resolver (e.g., Google DNS or ISP DNS) to perform a recursive lookup.
  - Prints all A records found for the given domain.

**Not Implemented:**
- **Advanced Error Recovery or Retry Mechanisms:**  
  - Basic exception handling is provided, but further recovery strategies (such as query retries on multiple servers) are not implemented.

---

## Design Decisions

- **Use of UDP for DNS Queries:**  
  - DNS queries are typically performed over UDP due to its low overhead. A timeout of 3 seconds is set (line defining `TIMEOUT`) to balance between waiting long enough for responses and failing fast in case of issues.

- **Iterative vs. Recursive Resolution:**  
  - **Iterative Resolution:**  
    - Follows the hierarchical DNS structure by starting at the root and moving down. This approach clearly demonstrates the iterative process and allows us to manually extract and resolve nameserver information.
  - **Recursive Resolution:**  
    - Leverages the system’s recursive resolver for simplicity and to compare results with the iterative method.



- **Design Decision for Timeout :**  
  - The timeout value (3 seconds) was chosen based on a trade-off between network variability and responsiveness. This value helps prevent the program from hanging too long on a non-responsive DNS server.

---

## Implementation

### High-Level Overview

The project is broken down into the following key functions:
- **`send_dns_query(server, domain)`**  
  - Constructs a DNS query message for an A record.
  - Sends the query over UDP and returns the response.
  
- **`extract_next_nameservers(response)`**  
  - Extracts NS records from the authority section.
  - Resolves NS hostnames to IP addresses to be used in subsequent queries.

- **`iterative_dns_lookup(domain)`**  
  - Manages the iterative resolution process.
  - Updates the resolution stage (ROOT → TLD → AUTH) as it queries nameservers until an answer is found.

- **`recursive_dns_lookup(domain)`**  
  - Uses the system’s resolver to perform recursive DNS resolution.

### Code Flow Diagram

Below is a simplified flow diagram representing the iterative DNS lookup process:

flowchart TD
    A[Start: Domain to Resolve] --> B[Set next_ns_list to Root Servers]
    B --> C{While next_ns_list not empty?}
    C -- Yes --> D[Pick first nameserver from list]
    D --> E[Send DNS Query via UDP]
    E --> F{Valid Response?}
    F -- Yes --> G{Response contains answer?}
    G -- Yes --> H[Print Answer and Exit]
    G -- No --> I[Extract NS records and resolve to IPs]
    I --> J[Update next_ns_list and Resolution Stage]
    J --> C
    F -- No --> K[Print Error and Exit]
    C -- No --> L[Print Resolution Failed]

### Code Flow (Iterative)

1.  **Initialization:**
    *   The lookup begins with a list of root server IPs.
2.  **Querying:**
    *   The code queries the first server from the list.
3.  **Processing Response:**
    *   If an answer is found, it prints the result.
    *   If not, it extracts NS records from the authority section and resolves them.
4.  **Stage Transition:**
    *   The resolution stage is updated from ROOT to TLD to AUTH.
5.  **Termination:**
    *   The process continues until a valid answer is obtained or no servers respond.

---

## Testing

### Correctness Testing

*   **Domain Verification:**
    *   Ran tests with known domains (e.g., `google.com`, `example.com`) and compared outputs with standard tools like `nslookup` and `dig`.
*   **Iterative vs. Recursive Comparison:**
    *   Ensured that both iterative and recursive methods produce consistent results for the same input.

### Stress Testing

*   **Timeout and Error Handling:**
    *   Tested scenarios with non-responsive DNS servers to ensure the timeout mechanism and error handling work correctly.
*   **Edge Cases:**
    *   Checked behavior for domains with no A records and for invalid domain inputs.

---

## Restrictions

*   **Resource Constraints:**
    *   **Message Size:**
        *   The maximum DNS message size is governed by the UDP protocol and the underlying DNS server configurations.


---

## Challenges

*   **Handling Network Unreliability:**
    *   Managing UDP timeouts and handling non-responsive servers required careful exception handling.
*   **NS Record Resolution:**
    *   Extracting NS records from the authority section and reliably converting them to IP addresses proved challenging due to intermittent resolution failures.
*   **Debugging Stages:**
    *   Ensuring correct transitions between ROOT, TLD, and AUTH stages required thorough testing and debugging.

---

## Contribution of Each Member

*   **Single Member Project:**
    
    *   100% contribution by the author.
    
    *   Tanishq Maheshwari(221128):     50%
    *   Apoorv Tandon(220192):          50% 
      

---

## Sources Referred

*   **dnspython Documentation:**
    *   [dnspython library documentation](http://www.dnspython.org/)
*   **Python Networking Tutorials:**
    *   Various online tutorials on DNS resolution and network programming.


---

## Declaration

I hereby declare that we did not indulge in plagiarism and that this assignment represents our original work.

---

## Feedback

*   **Positive Aspects:**
    *   The assignment provided an excellent opportunity to delve into DNS protocols and network programming.
*   **General Comments:**
    *   The challenges faced during the project enhanced my debugging skills and deepened my understanding of the DNS resolution process.
