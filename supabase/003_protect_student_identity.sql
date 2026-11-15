-- Migration 003: Protect Student Identity Records
-- Ensures that students can only update their contact number, gmail, and year of study.
-- Name and roll number are verified college records and can only be updated by a Manager.

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
