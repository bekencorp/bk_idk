#!/usr/bin/env python3
"""
Unified logging utility for bksecure tools.
Provides structured logging with different verbosity levels.
"""

import logging
import sys
from enum import IntEnum

class LogLevel(IntEnum):
    """Log verbosity levels"""
    QUIET = 0      # Only errors
    SIMPLE = 1     # Key steps and errors (default)
    NORMAL = 2     # Normal info + errors
    VERBOSE = 3    # All info + debug
    DEBUG = 4      # Everything including debug

# Global log level
_current_log_level = LogLevel.SIMPLE

class StructuredFormatter(logging.Formatter):
    """Custom formatter for structured logging"""
    
    # Color codes for terminal
    COLORS = {
        'DEBUG': '\033[36m',      # Cyan
        'INFO': '\033[32m',       # Green
        'WARNING': '\033[33m',    # Yellow
        'ERROR': '\033[31m',      # Red
        'CRITICAL': '\033[35m',   # Magenta
        'RESET': '\033[0m'
    }
    
    # Step markers
    STEP_START = '>>>'
    STEP_END = '<<<'
    STEP_INFO = '---'
    STEP_ERROR = '!!!'
    
    def __init__(self, use_color=True, show_level=True):
        self.use_color = use_color and sys.stdout.isatty()
        self.show_level = show_level
        super().__init__()
    
    def format(self, record):
        # Determine prefix based on message content
        prefix = ''
        if hasattr(record, 'step_start'):
            prefix = self.STEP_START
        elif hasattr(record, 'step_end'):
            prefix = self.STEP_END
        elif hasattr(record, 'step_info'):
            prefix = self.STEP_INFO
        elif record.levelno >= logging.ERROR:
            prefix = self.STEP_ERROR
        
        # Format message
        if prefix:
            msg = f'{prefix} {record.getMessage()}'
        else:
            msg = record.getMessage()
        
        # Add color if enabled
        if self.use_color and record.levelname in self.COLORS:
            color = self.COLORS[record.levelname]
            reset = self.COLORS['RESET']
            msg = f'{color}{msg}{reset}'
        
        # Add level if requested
        if self.show_level and record.levelno >= logging.WARNING:
            level = record.levelname[:4]
            return f'[{level}] {msg}'
        
        return msg

def setup_logging(log_level=LogLevel.SIMPLE):
    """
    Setup logging with specified verbosity level.
    
    Args:
        log_level: LogLevel enum value (QUIET, SIMPLE, NORMAL, VERBOSE, DEBUG)
    """
    global _current_log_level
    _current_log_level = log_level
    
    # Clear existing handlers
    root_logger = logging.getLogger()
    root_logger.handlers.clear()
    
    # Create handler
    handler = logging.StreamHandler(sys.stdout)
    
    # Set formatter based on log level
    if log_level >= LogLevel.VERBOSE:
        formatter = StructuredFormatter(use_color=True, show_level=True)
        handler.setLevel(logging.DEBUG)
        root_logger.setLevel(logging.DEBUG)
    elif log_level >= LogLevel.NORMAL:
        formatter = StructuredFormatter(use_color=True, show_level=False)
        handler.setLevel(logging.INFO)
        root_logger.setLevel(logging.INFO)
    elif log_level >= LogLevel.SIMPLE:
        formatter = StructuredFormatter(use_color=True, show_level=False)
        handler.setLevel(logging.INFO)
        root_logger.setLevel(logging.INFO)
    else:  # QUIET
        formatter = StructuredFormatter(use_color=False, show_level=True)
        handler.setLevel(logging.ERROR)
        root_logger.setLevel(logging.ERROR)
    
    handler.setFormatter(formatter)
    root_logger.addHandler(handler)
    
    # Suppress verbose debug logs from third-party libraries
    if log_level < LogLevel.DEBUG:
        logging.getLogger('urllib3').setLevel(logging.WARNING)
        logging.getLogger('requests').setLevel(logging.WARNING)

def get_log_level():
    """Get current log level"""
    return _current_log_level

def should_log_debug():
    """Check if DEBUG logs should be shown"""
    return _current_log_level >= LogLevel.DEBUG

def should_log_verbose():
    """Check if VERBOSE logs should be shown"""
    return _current_log_level >= LogLevel.VERBOSE

def log_step_start(step_name, details=None):
    """Log the start of a major step"""
    logger = logging.getLogger()
    extra = {'step_start': True}
    if details:
        logger.info(f'Starting: {step_name} - {details}', extra=extra)
    else:
        logger.info(f'Starting: {step_name}', extra=extra)

def log_step_end(step_name, status='OK', details=None):
    """Log the end of a major step"""
    logger = logging.getLogger()
    extra = {'step_end': True}
    if status == 'OK':
        if details:
            logger.info(f'Completed: {step_name} - {details}', extra=extra)
        else:
            logger.info(f'Completed: {step_name}', extra=extra)
    else:
        if details:
            logger.error(f'Failed: {step_name} - {details}', extra=extra)
        else:
            logger.error(f'Failed: {step_name}', extra=extra)

def log_step_info(message):
    """Log step information"""
    logger = logging.getLogger()
    extra = {'step_info': True}
    logger.info(message, extra=extra)

def log_progress(message, level=logging.INFO):
    """Log progress information"""
    logger = logging.getLogger()
    if _current_log_level >= LogLevel.NORMAL or level >= logging.WARNING:
        logger.log(level, message)

def log_debug(message, force=False):
    """Log debug message (only if verbose mode)"""
    if force or should_log_debug():
        logger = logging.getLogger()
        logger.debug(message)

def log_verbose(message):
    """Log verbose message (only if verbose mode)"""
    if should_log_verbose():
        logger = logging.getLogger()
        logger.info(message)
