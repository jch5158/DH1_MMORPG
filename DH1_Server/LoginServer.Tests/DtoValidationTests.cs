using LoginServer.DTOs.Auth;
using System.ComponentModel.DataAnnotations;

namespace LoginServer.Tests;

public class DtoValidationTests
{
    private static List<ValidationResult> ValidateModel(object model)
    {
        var results = new List<ValidationResult>();
        var context = new ValidationContext(model);
        Validator.TryValidateObject(model, context, results, validateAllProperties: true);
        return results;
    }

    #region LoginRequest

    [Fact]
    public void LoginRequest_Valid_NoErrors()
    {
        var dto = new LoginRequest { Email = "test@example.com", Password = "ValidPass1" };
        var errors = ValidateModel(dto);
        Assert.Empty(errors);
    }

    [Fact]
    public void LoginRequest_InvalidEmail_HasError()
    {
        var dto = new LoginRequest { Email = "not-an-email", Password = "ValidPass1" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Email"));
    }

    [Fact]
    public void LoginRequest_PasswordTooShort_HasError()
    {
        var dto = new LoginRequest { Email = "test@example.com", Password = "short" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Password"));
    }

    [Fact]
    public void LoginRequest_PasswordTooLong_HasError()
    {
        var dto = new LoginRequest { Email = "test@example.com", Password = new string('a', 129) };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Password"));
    }

    #endregion

    #region RegisterRequest

    [Fact]
    public void RegisterRequest_Valid_NoErrors()
    {
        var dto = new RegisterRequest { Email = "test@example.com", Password = "ValidPass1" };
        var errors = ValidateModel(dto);
        Assert.Empty(errors);
    }

    [Fact]
    public void RegisterRequest_InvalidEmail_HasError()
    {
        var dto = new RegisterRequest { Email = "bad-email", Password = "ValidPass1" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Email"));
    }

    [Fact]
    public void RegisterRequest_PasswordTooShort_HasError()
    {
        var dto = new RegisterRequest { Email = "test@example.com", Password = "short" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Password"));
    }

    #endregion

    #region ForgotPasswordRequest

    [Fact]
    public void ForgotPasswordRequest_Valid_NoErrors()
    {
        var dto = new ForgotPasswordRequest { Email = "test@example.com" };
        var errors = ValidateModel(dto);
        Assert.Empty(errors);
    }

    [Fact]
    public void ForgotPasswordRequest_InvalidEmail_HasError()
    {
        var dto = new ForgotPasswordRequest { Email = "bad" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Email"));
    }

    #endregion

    #region VerifyCodeRequest

    [Fact]
    public void VerifyCodeRequest_Valid_NoErrors()
    {
        var dto = new VerifyCodeRequest { Email = "test@example.com", VerifyCode = "123456" };
        var errors = ValidateModel(dto);
        Assert.Empty(errors);
    }

    [Fact]
    public void VerifyCodeRequest_CodeTooShort_HasError()
    {
        var dto = new VerifyCodeRequest { Email = "test@example.com", VerifyCode = "123" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("VerifyCode"));
    }

    [Fact]
    public void VerifyCodeRequest_CodeWithLetters_HasError()
    {
        var dto = new VerifyCodeRequest { Email = "test@example.com", VerifyCode = "12ab56" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("VerifyCode"));
    }

    [Fact]
    public void VerifyCodeRequest_CodeTooLong_HasError()
    {
        var dto = new VerifyCodeRequest { Email = "test@example.com", VerifyCode = "1234567" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("VerifyCode"));
    }

    #endregion

    #region ResetPasswordRequest

    [Fact]
    public void ResetPasswordRequest_Valid_NoErrors()
    {
        var dto = new ResetPasswordRequest { Email = "test@example.com", VerifyCode = "123456", ResetPassword = "NewPass123!" };
        var errors = ValidateModel(dto);
        Assert.Empty(errors);
    }

    [Fact]
    public void ResetPasswordRequest_InvalidEmail_HasError()
    {
        var dto = new ResetPasswordRequest { Email = "bad", VerifyCode = "123456", ResetPassword = "NewPass123!" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("Email"));
    }

    [Fact]
    public void ResetPasswordRequest_CodeNotDigits_HasError()
    {
        var dto = new ResetPasswordRequest { Email = "test@example.com", VerifyCode = "abcdef", ResetPassword = "NewPass123!" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("VerifyCode"));
    }

    [Fact]
    public void ResetPasswordRequest_PasswordTooShort_HasError()
    {
        var dto = new ResetPasswordRequest { Email = "test@example.com", VerifyCode = "123456", ResetPassword = "short" };
        var errors = ValidateModel(dto);
        Assert.Contains(errors, e => e.MemberNames.Contains("ResetPassword"));
    }

    #endregion
}
