
# Authentication with Auth0

1. Create a new Auth0 application (Single page app)
2. Create a new Auth0 API:
   1. Choose `RS256` signing algorithm
   2. Set identifier (audience) to `https://sist2`
3. Download the Auth0 certificate from https://<domain>.auth0.com/pem (you can find the domain Applications->Basic information)
4. Extract the public key from the certificate using `openssl x509 -pubkey -noout -in cert.pem > pubkey.txt`
5. Start the sist2 web server

Example options:
```bash
sist2 web \
  --auth0-client-id XXX \
  --auth0-audience https://sist2 \
  --auth0-domain YYY.auth0.com \
  --auth0-public-key-file /ZZZ/pubkey.txt
```