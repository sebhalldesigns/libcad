# libcad Document Model

## Aims 
- Extremely generic - don't want to lock in any workflows or use-cases
- Git / filesystem friendly - should be text-based and diffable
- Scalable - should be able to handle large documents without performance issues
- Extensible - should be easy to add new features and capabilities without breaking existing documents

## Concepts
- Object-oriented model - all objects are instances of classes, which have properties (per-instance data) and methods (functions that operate on the instance data)
- File-oriented referencing - documents reference other documents by file path, rather than embedding data directly.


## Performance Notes
- Possible local caching (i.e store binary data such as meshes etc in a separate file that isn't source controlled)
- Lazy loading of data - only load data when it's needed
