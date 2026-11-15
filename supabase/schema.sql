-- College Lost & Found: Supabase schema.
-- Run this whole file once in: Supabase dashboard > SQL Editor > New query > Run.
-- It is safe to run again (everything is "if not exists" / "or replace").

-- 1. One generic document table. The app stores every "collection" in it (items, claims, profiles ...).
create table if not exists public.docs (
  col        text        not null,
  id         text        not null,
  data       jsonb       not null default '{}'::jsonb,
  updated_at timestamptz not null default now(),
  primary key (col, id)
);
create index if not exists docs_claims_student_idx on public.docs ((data->>'studentId')) where col = 'claims';

-- 2. Helpers. lf_is_admin() is SECURITY DEFINER so the policies can look up the allowlist without recursing.
create or replace function public.lf_email() returns text
language sql stable as $$ select lower(coalesce(auth.jwt() ->> 'email', '')) $$;

create or replace function public.lf_is_admin() returns boolean
language sql stable security definer set search_path = public as $$
  select public.lf_email() <> ''
     and (
       public.lf_email() = 'tharanesvaran07@gmail.com'
       or exists (select 1 from public.docs where col = 'adminAllowlist' and id = public.lf_email())
     );
$$;

create or replace function public.lf_is_manager() returns boolean
language sql stable security definer set search_path = public as $$
  select public.lf_email() <> ''
     and (
       public.lf_email() = 'tharanesvaran07@gmail.com'
       or exists (
         select 1 from public.docs 
         where col = 'adminAllowlist' 
           and id = public.lf_email() 
           and data ->> 'role' = 'manager'
       )
     );
$$;

-- 3. Partial update used by the app: shallow-merges a JSON patch into a document (RLS still applies).
create or replace function public.doc_update(p_col text, p_id text, p_patch jsonb) returns void
language plpgsql security invoker set search_path = public as $$
begin
  update public.docs set data = data || p_patch, updated_at = now() where col = p_col and id = p_id;
  if not found then raise exception 'document not found or not allowed'; end if;
end $$;

-- 4. Row level security. Nobody signed out can read or write anything.
alter table public.docs enable row level security;
revoke all on public.docs from anon;
grant select, insert, update, delete on public.docs to authenticated;
grant execute on function public.doc_update(text, text, jsonb) to authenticated;
grant execute on function public.lf_is_admin() to authenticated;
grant execute on function public.lf_is_manager() to authenticated;

drop policy if exists docs_select on public.docs;
drop policy if exists docs_insert on public.docs;
drop policy if exists docs_update on public.docs;
drop policy if exists docs_delete on public.docs;

-- Read: staff read everything. Students read the public item list + settings, their own claims,
-- their own per-student documents, and their own report area. Everyone may read their own allowlist row (to detect staff).
create policy docs_select on public.docs for select to authenticated using (
  public.lf_is_admin()
  or col in ('items', 'settings')
  or (col = 'adminAllowlist' and id = public.lf_email())
  or (col = 'claims' and data ->> 'studentId' = auth.uid()::text)
  or (col in ('profiles', 'claimAnswers', 'returns', 'lost', 'found') and id = auth.uid()::text)
  or col = 'data/users/' || auth.uid()::text
);

-- Create: Managers may add to adminAllowlist. Students may file claims, profile, answers, reports.
create policy docs_insert on public.docs for insert to authenticated with check (
  (public.lf_is_manager() and col = 'adminAllowlist')
  or (public.lf_is_admin() and col <> 'adminAllowlist')
  or (col = 'claims' and data ->> 'studentId' = auth.uid()::text and data ->> 'status' = 'requested')
  or (col in ('profiles', 'claimAnswers', 'lost', 'found') and id = auth.uid()::text)
  or col = 'data/users/' || auth.uid()::text
);

-- Change: Managers can update adminAllowlist. Admins update items/claims. Students update their own profile/answers/reports.
create policy docs_update on public.docs for update to authenticated using (
  (public.lf_is_manager() and col = 'adminAllowlist' and id <> 'tharanesvaran07@gmail.com')
  or (public.lf_is_admin() and col <> 'adminAllowlist')
  or (col in ('profiles', 'claimAnswers', 'lost', 'found') and id = auth.uid()::text)
  or col = 'data/users/' || auth.uid()::text
) with check (
  (public.lf_is_manager() and col = 'adminAllowlist')
  or (public.lf_is_admin() and col <> 'adminAllowlist')
  or (col in ('profiles', 'claimAnswers', 'lost', 'found') and id = auth.uid()::text)
  or col = 'data/users/' || auth.uid()::text
);

-- Delete: Managers can remove admins (except primary manager) and students. Admins can remove items.
create policy docs_delete on public.docs for delete to authenticated using (
  (public.lf_is_manager() and col = 'adminAllowlist' and id <> 'tharanesvaran07@gmail.com')
  or (public.lf_is_admin() and col <> 'adminAllowlist')
);

-- 5. Live updates (realtime). Leave REPLICA IDENTITY at its default so deleted rows only expose their key.
do $$ begin
  if not exists (select 1 from pg_publication_tables where pubname = 'supabase_realtime' and schemaname = 'public' and tablename = 'docs') then
    alter publication supabase_realtime add table public.docs;
  end if;
end $$;

-- 6. STAFF / ADMIN ACCESS.
-- tharanesvaran07@gmail.com is permanently configured as the primary administrator.
-- To add new staff / admin accounts in Supabase, run this:
-- insert into public.docs (col, id, data) values
--   ('adminAllowlist', 'staff.email@gmail.com', '{"role": "admin"}')
-- on conflict (col, id) do update set data = excluded.data;

-- 7. STUDENT PROFILE PERMISSION SAFEGUARDS
-- Ensures that students can only update their contact number, gmail, and year of study.
-- Name and roll number are locked to verified records and can only be updated by a Manager.
create or replace function public.lf_protect_student_identity()
returns trigger as $$
begin
  -- When updating a profile document, if the actor is NOT a manager:
  if (new.col = 'profiles' or new.col like 'data/users/%') and not public.lf_is_manager() then
    -- Preserve verified name, roll, and dept from old record if present
    if old.data is not null then
      if old.data ? 'name' then
        new.data = jsonb_set(new.data, '{name}', old.data->'name');
      end if;
      if old.data ? 'roll' then
        new.data = jsonb_set(new.data, '{roll}', old.data->'roll');
      end if;
      if old.data ? 'dept' then
        new.data = jsonb_set(new.data, '{dept}', old.data->'dept');
      end if;
    end if;
  end if;
  return new;
end;
$$ language plpgsql security definer;

drop trigger if exists tr_protect_student_identity on public.docs;
create trigger tr_protect_student_identity
before update on public.docs
for each row
execute function public.lf_protect_student_identity();

