import path from 'node:path';
import logger from './logger';

/**
 * Validates if the target path is strictly within the allowed parent directory.
 * Prevents Path Traversal attacks and satisfies static analysis (CodeQL).
 * 
 * @param inputPath - The potentially untrusted path (relative or absolute)
 * @param allowedDir - The root directory allowed for access
 * @param errorMessage - Optional custom error message
 * @returns The absolute, normalized, and validated path
 * @throws Error if the path is invalid or outside the allowed directory
 */
export function getSafePath(inputPath: string, allowedDir: string, errorMessage = 'Access denied'): string {
  const resolvedPath = path.resolve(inputPath);
  const resolvedAllowedDir = path.resolve(allowedDir);
  
  // Windows case-insensitivity handling
  const isWindows = process.platform === 'win32';
  const checkPath = isWindows ? resolvedPath.toLowerCase() : resolvedPath;
  const checkAllowedDir = (isWindows ? resolvedAllowedDir.toLowerCase() : resolvedAllowedDir);
  
  // Ensure the base directory string ends with a separator to prevent 'parentdir-injection'
  // e.g. /home/user matches /home/user-extra without this.
  const checkAllowedDirWithSep = checkAllowedDir.endsWith(path.sep) 
    ? checkAllowedDir 
    : checkAllowedDir + path.sep;

  if (!checkPath.startsWith(checkAllowedDirWithSep) && checkPath !== checkAllowedDir) {
    logger.error('Security Alert: Path violation detected', { 
      inputPath, 
      resolvedPath, 
      allowedDir: resolvedAllowedDir 
    });
    throw new Error(errorMessage);
  }

  return resolvedPath;
}
