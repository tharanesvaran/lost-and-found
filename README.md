# College Lost & Found

Static site (`index.html` + `config.js`) backed by Supabase (database + Google sign-in). Deploys to Vercel with no build step.

## 1. Supabase
1. Create a project at supabase.com.
2. **SQL Editor** > paste all of `supabase/schema.sql` > Run.
3. Add your staff accounts: edit and run the `insert into public.docs ... 'adminAllowlist'` example at the bottom of that file
   (use the staff member's Google email, lowercase; the username must end with `.admin`).
4. **Project Settings > API**: copy the Project URL and the `anon` / publishable key into `config.js`.

## 2. Google sign-in
1. [Google Cloud Console](https://console.cloud.google.com/) > APIs & Services > **OAuth consent screen** (External) > publish it.
2. **Credentials > Create credentials > OAuth client ID > Web application**.
   Under *Authorised redirect URIs* add exactly: `https://<your-project-ref>.supabase.co/auth/v1/callback`
3. Copy the Client ID and Client secret into Supabase > **Authentication > Providers > Google** > enable > Save.
4. Supabase > **Authentication > URL Configuration**:
   - **Site URL**: your Vercel URL, e.g. `https://your-app.vercel.app`
   - **Redirect URLs**: add `https://your-app.vercel.app/**` and `http://localhost:3000/**` (and your custom domain if you have one).
   Without this, Google sign-in returns to the wrong place or fails.

## 3. GitHub + Vercel
1. Push this folder to a GitHub repo.
2. Vercel > Add New Project > import the repo. Framework preset: **Other**. No build command, no output directory. Deploy.
3. Open the Vercel URL and sign in with Google.

Vercel preview URLs (random subdomains) are different origins; add them to the Supabase Redirect URLs if you want to test sign-in on them.

## Notes
- `config.js` holds only the public anon key. Data is protected by the row-level-security rules in `schema.sql`, not by hiding the key.
  Never commit the `service_role` key.
- Until `config.js` is filled in, the page runs in demo mode (data lives in the open tab only).
- Local test: `npx serve .` (add `http://localhost:3000/**` to Redirect URLs).
- Matching is plain JavaScript inside `index.html` (`findMatches`). A suggestion needs a score of 9 or more out of 12 (75%).
