# Security Policy

## Scope

This repository contains **benign lab code** (a PING/VERSION driver) and **defensive documentation**. It intentionally does not ship real vulnerable drivers, kdmapper, or a working manual-mapping/hiding implementation.

## Reporting

If you find:

- a bug in the benign sample that could cause memory corruption or privilege escalation;
- an accidental inclusion of a sensitive binary, certificate, credential, or target-specific offset;
- a detection rule with a dangerous false-positive/false-negative rate;

please open a private security advisory or contact the maintainer out-of-band. Do not open a public issue containing exploit code or live credentials.

## Safe test policy

- Test only in an isolated VM with a snapshot.
- Never load the sample on a production host.
- Use a throwaway test certificate and remove it after testing.
- If a test causes a BSOD, capture the bugcheck code and roll back the snapshot.
