#!/usr/bin/env stack
{- stack --resolver=lts-10.1 --install-ghc runghc --package typed-process
    --package optparse-applicative --package path -}
{-#LANGUAGE OverloadedStrings #-}
{-#LANGUAGE GADTs #-}
--import System.IO (hPutStr, hClose)
import System.Process.Typed
import qualified Data.ByteString.Lazy as L
--import qualified Data.ByteString.Lazy.Char8 as L8
--import Control.Concurrent.STM (atomically)
--import Control.Exception (throwIO)
import Options.Applicative
import Data.Semigroup ((<>))
import Path
import System.Exit (exitWith, ExitCode(..), exitFailure)

data Command =
    CompareRawOutput FilePath FilePath FilePath
  | CompareSnaOutput FilePath -- TODO
  deriving (Show)

newtype Options = Options Command


main :: IO ()
main = run =<< execParser
    (parseOptions `withInfo` "Run tests")

parseOptions :: Parser Options
parseOptions = Options <$> parseCommand

run :: Options -> IO ()
run (Options cmd) = case cmd of
    CompareRawOutput asm fn destpath -> doCompareRawOutput asm fn destpath
    CompareSnaOutput fn -> putStrLn fn

withInfo :: Parser a -> String -> ParserInfo a
withInfo opts desc = info (helper <*> opts) $ progDesc desc

parseCompareRawOutput :: Parser Command
parseCompareRawOutput = CompareRawOutput
  <$> strArgument (metavar "ASM" <> help "Full path to sjasmplus")
  <*> strArgument (metavar "SOURCE" <> help "Full path to *.asm file")
  <*> strArgument (metavar "DESTDIR" <> help "Output directory")

parseCompareSnaOutput :: Parser Command
parseCompareSnaOutput = CompareSnaOutput
  <$> argument str (metavar "SOURCE")

parseCommand :: Parser Command
parseCommand = subparser $
    command "raw" (parseCompareRawOutput
    `withInfo` "Assemble a file and compare *.out with a specimen") <>
    command "sna" (parseCompareSnaOutput
    `withInfo` "Assemble a file and compare *.out with a specimen")

doCompareRawOutput :: FilePath -> FilePath -> FilePath -> IO ()
doCompareRawOutput a f p = do
  asm <- parseAbsFile a
  asmFile <- parseAbsFile f
  destDir <- parseAbsDir p
  -- putStrLn $ show asm ++ ", " ++ show afile ++ ", " ++ show adest
  specimen <- setFileExtension "out" asmFile
  let outFile = destDir </> filename specimen
  exitCode <- runProcess $ proc (toFilePath asm)
    ["--raw=" ++ toFilePath outFile, toFilePath asmFile]
  case exitCode of
    ExitSuccess -> compareFiles specimen outFile
    _ -> exitWith exitCode

compareFiles :: Path Abs File -> Path Abs File -> IO ()
compareFiles f1 f2 = do
  c1 <- L.readFile $ toFilePath f1
  c2 <- L.readFile $ toFilePath f2
  if c1 == c2
  then putStrLn "Output mathes the specimen"
  else do
    putStrLn "Output does not match the specimen"
    exitFailure
