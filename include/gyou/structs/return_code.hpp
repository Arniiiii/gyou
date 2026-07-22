#ifndef INCLUDE_STRUCTS_RETURN_CODE_HPP_
#define INCLUDE_STRUCTS_RETURN_CODE_HPP_

namespace gyou
{

    // NOLINTNEXTLINE(performance-enum-size)
    enum class ReturnCode : int
    {
        Success = 0,
        PartialSuccess = 1,
        AllHaveFailed = 2,
        NoEbuildFound = 3,
        FailDuringParsingCmdValues = 4,
        FailSpecifiedValueIsIncorrect = 5,
        FailDuringInitializationConfig = 6,
        FailStandardException = 7,
        FailParsePromptResult = 8,
        FailInitializationLogger = 9,
        ReceivedCancellationSignal = 10,
        FailedReadingGroupCiFile = 11,
        FailedParsingGroupCiFile = 12,
        FailedCreateDirTmpWorktrees = 13,
        FailedToFindGit = 14,
        FailedToFindGh = 15,
        FailedMakingGitToMakeWorktrees = 16,
        FailedApplyChange = 17,
        FailedGhIsNotLoggedIn = 18,

    };
}  // namespace gyou

#endif  // INCLUDE_STRUCTS_RETURN_CODE_HPP_
