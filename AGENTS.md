# Repository Guidelines

## Project Structure & Module Organization
- `frontend/`: React + TypeScript + Vite UI. Components live in `frontend/src/components/`, shared types in `frontend/src/types/`.
- `api-gateway/`: Express API gateway with routes in `api-gateway/src/routes/` and services in `api-gateway/src/services/`.
- `api-gateway/tests/` and `frontend/src/components/__tests__/`: automated tests.
- `shared_volume/`: runtime uploads/results used by the API (treat as generated output).
- `docs/`: project guides and references.

## Build, Test, and Development Commands
Run commands from the relevant package directory:
- `frontend/`
  - `npm install`: install UI dependencies.
  - `npm run dev`: start Vite dev server.
  - `npm run build`: type-check and build production assets.
  - `npm run test`: run Vitest.
- `api-gateway/`
  - `npm install`: install API dependencies.
  - `npm run dev`: start API with nodemon.
  - `npm start`: run the API server.
  - `npm run test`: run Jest tests.

## Coding Style & Naming Conventions
- Indentation: 2 spaces, include semicolons.
- Frontend: React components in PascalCase (`Hero.tsx`), hooks and helpers in camelCase.
- Backend: routes/services/utilities in camelCase file names (`analysisService.js`).
- No repo-wide formatter or linter is configured; follow existing patterns in touched files.

## Testing Guidelines
- Frontend uses Vitest + Testing Library; tests are named `*.test.tsx` under `frontend/src/components/__tests__/`.
- API uses Jest + Supertest; tests live in `api-gateway/tests/` as `*.test.js`.
- Keep tests focused on public behavior (routes/components) and run them before PRs.

## Commit & Pull Request Guidelines
- History favors short, imperative messages (e.g., "Update dependencies", "install axios").
- PRs should include: a brief summary, testing notes (`npm run test`), and screenshots for UI changes.
- Link related issues or docs when available.

## Configuration & Data
- API configuration uses environment variables (see `PORT` in `api-gateway/server.js`); document new vars in PRs.
- Avoid committing new files under `shared_volume/` unless they are intentional fixtures.
