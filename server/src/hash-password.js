import { hashPassword } from "./auth.js";

const password = process.argv[2] ?? process.env.ADMIN_PASSWORD;
if (!password) {
  console.error("Usage: npm run hash-password -- <password>");
  process.exit(1);
}

console.log(await hashPassword(password));
