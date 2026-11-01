-- PostgreSQL schema for job storage.

CREATE TABLE IF NOT EXISTS jobs (
    id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    command         TEXT NOT NULL CHECK (length(command) > 0),
    status          TEXT NOT NULL DEFAULT 'QUEUED'
                    CHECK (status IN ('QUEUED', 'RUNNING', 'COMPLETED', 'FAILED')),

    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    started_at      TIMESTAMPTZ,
    finished_at     TIMESTAMPTZ,

    -- Set while a worker owns a RUNNING job. Workers should renew the lease
    -- for long-running jobs. An expired lease makes the job eligible to retry.
    worker_id       TEXT,
    lease_until     TIMESTAMPTZ,

    -- The most recent worker to claim the job (host-pid-index). Unlike
    -- worker_id, it is kept after the job finishes.
    last_worker_id  TEXT,

    -- Increment attempts each time a worker claims the job.
    attempts        INTEGER NOT NULL DEFAULT 0 CHECK (attempts >= 0),
    max_attempts    INTEGER NOT NULL DEFAULT 3 CHECK (max_attempts > 0),

    exit_code       INTEGER,
    error_text      TEXT,

    CONSTRAINT jobs_running_has_lease CHECK (
        status <> 'RUNNING'
        OR (worker_id IS NOT NULL AND lease_until IS NOT NULL)
    ),
    CONSTRAINT jobs_not_running_has_no_lease CHECK (
        status = 'RUNNING'
        OR (worker_id IS NULL AND lease_until IS NULL)
    ),
    CONSTRAINT jobs_terminal_has_finished_at CHECK (
        status NOT IN ('COMPLETED', 'FAILED')
        OR finished_at IS NOT NULL
    ),
    CONSTRAINT jobs_nonterminal_has_no_finished_at CHECK (
        status IN ('COMPLETED', 'FAILED')
        OR finished_at IS NULL
    )
);

-- Supports FIFO selection of queued work. The worker claim operation should
-- still lock selected rows with FOR UPDATE SKIP LOCKED inside a transaction.
CREATE INDEX IF NOT EXISTS jobs_queued_created_idx
    ON jobs (created_at, id)
    WHERE status = 'QUEUED';

-- Supports finding jobs whose worker lease has expired.
CREATE INDEX IF NOT EXISTS jobs_expired_lease_idx
    ON jobs (lease_until, id)
    WHERE status = 'RUNNING';

-- Supports looking up a worker's currently assigned jobs for lease renewal.
CREATE INDEX IF NOT EXISTS jobs_worker_running_idx
    ON jobs (worker_id, lease_until)
    WHERE status = 'RUNNING';
