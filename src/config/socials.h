#pragma once
#include <vector>
#include <string>

struct SocialLink {
    std::string name;
    std::string url;
    std::string icon_name;
    std::string subtitle;
};

const std::vector<SocialLink> MY_SOCIALS = {
    {
        "GitHub", 
        "https://github.com/acedmicabhishek", 
        "github-symbolic",
        "Check out my open source projects."
    },
    {
        "LinkedIn", 
        "https://www.linkedin.com/in/abhishek-anand-089709362/", 
        "user-info-symbolic",
        "Connect with me professionally."
    },
    {
        "KernelDrive Repo",
        "https://github.com/acedmicabhishek/KernelDrive",
        "system-software-install-symbolic",
        "Star the project on GitHub!"
    }
};
